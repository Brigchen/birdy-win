#include "infer/BirdEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>

#ifdef BIRDY_HAS_NCNN
#include <ncnn/mat.h>
#include <ncnn/net.h>
#endif

namespace birdy {
namespace {

constexpr int kDetSize = 640;
constexpr int kClsSize = 224;
constexpr int kClsShort = 256;
constexpr int kCropMargin = 10;
constexpr int kMaxDetections = 10;
constexpr int kBirdClass = 14;
constexpr int kCocoClasses = 80;
constexpr float kPad = 114.f / 255.f;
// ImageNet，按 B,G,R 存放；RGB 通道 c 取 [2-c]。与手机端 ncnn 认种一致。
constexpr float kBgrMean[3] = {0.406f, 0.456f, 0.485f};
constexpr float kBgrStd[3] = {0.225f, 0.224f, 0.229f};

bool parseBirdInfo(const std::string& path, std::vector<SpeciesName>* out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    const std::string text = buf.str();
    out->clear();
    std::vector<std::string> cur;
    std::string token;
    bool inStr = false;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (!inStr) {
            if (c == '"') {
                inStr = true;
                token.clear();
            }
            continue;
        }
        if (c == '\\' && i + 1 < text.size()) {
            const char n = text[++i];
            switch (n) {
            case '"': token.push_back('"'); break;
            case '\\': token.push_back('\\'); break;
            case 'n': token.push_back('\n'); break;
            case 't': token.push_back('\t'); break;
            case 'r': token.push_back('\r'); break;
            default: token.push_back(n); break;
            }
            continue;
        }
        if (c == '"') {
            inStr = false;
            cur.push_back(token);
            if (cur.size() == 3) {
                out->push_back(SpeciesName{cur[0], cur[1], cur[2]});
                cur.clear();
            }
            continue;
        }
        token.push_back(c);
    }
    return !out->empty();
}

float iouOrContain(const Detection& a, const Detection& b) {
    const float ix1 = std::max(a.x1, b.x1);
    const float iy1 = std::max(a.y1, b.y1);
    const float ix2 = std::min(a.x2, b.x2);
    const float iy2 = std::min(a.y2, b.y2);
    const float iw = std::max(0.f, ix2 - ix1);
    const float ih = std::max(0.f, iy2 - iy1);
    const float inter = iw * ih;
    if (inter <= 0.f) {
        return 0.f;
    }
    const float aa = (a.x2 - a.x1) * (a.y2 - a.y1);
    const float ba = (b.x2 - b.x1) * (b.y2 - b.y1);
    const float uni = aa + ba - inter;
    const float iou = uni > 1e-6f ? inter / uni : 0.f;
    const float smaller = std::min(aa, ba);
    const float contain = smaller > 1e-6f ? inter / smaller : 0.f;
    return std::max(iou, contain >= 0.8f ? 1.f : 0.f);
}

std::vector<Detection> runNms(std::vector<Detection> dets, float thr) {
    std::sort(dets.begin(), dets.end(), [](const Detection& a, const Detection& b) {
        return a.detScore > b.detScore;
    });
    std::vector<Detection> kept;
    for (const Detection& d : dets) {
        bool dup = false;
        for (const Detection& k : kept) {
            if (iouOrContain(d, k) >= thr) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            kept.push_back(d);
        }
        if (static_cast<int>(kept.size()) >= kMaxDetections) {
            break;
        }
    }
    return kept;
}

void sampleBilinear(const std::uint8_t* src, int w, int h, float x, float y,
                     float* b, float* g, float* r) {
    x = std::max(0.f, std::min(x, static_cast<float>(w - 1)));
    y = std::max(0.f, std::min(y, static_cast<float>(h - 1)));
    const int x0 = static_cast<int>(x);
    const int y0 = static_cast<int>(y);
    const int x1 = std::min(x0 + 1, w - 1);
    const int y1 = std::min(y0 + 1, h - 1);
    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);
    auto pix = [&](int px, int py, int c) {
        return src[(static_cast<size_t>(py) * w + px) * 3 + c] / 255.f;
    };
    auto mix = [&](int c) {
        const float v00 = pix(x0, y0, c);
        const float v10 = pix(x1, y0, c);
        const float v01 = pix(x0, y1, c);
        const float v11 = pix(x1, y1, c);
        const float a = v00 + (v10 - v00) * tx;
        const float b2 = v01 + (v11 - v01) * tx;
        return a + (b2 - a) * ty;
    };
    *b = mix(0);
    *g = mix(1);
    *r = mix(2);
}

struct Letterbox {
    std::vector<float> bgr;
    float scale = 1.f;
    float padW = 0.f;
    float padH = 0.f;
};

Letterbox makeLetterbox(const std::uint8_t* src, int w, int h, int side) {
    Letterbox box;
    box.bgr.assign(static_cast<size_t>(3) * side * side, kPad);
    if (w < 1 || h < 1) {
        return box;
    }
    box.scale = std::min(static_cast<float>(side) / w, static_cast<float>(side) / h);
    const int nw = std::max(1, std::min(side, static_cast<int>(std::lround(w * box.scale))));
    const int nh = std::max(1, std::min(side, static_cast<int>(std::lround(h * box.scale))));
    box.padW = (side - nw) * 0.5f;
    box.padH = (side - nh) * 0.5f;
    const int x0 = static_cast<int>(std::floor(box.padW));
    const int y0 = static_cast<int>(std::floor(box.padH));
    for (int y = 0; y < nh; ++y) {
        const float sy = (static_cast<float>(y) + 0.5f) / box.scale - 0.5f;
        for (int x = 0; x < nw; ++x) {
            const float sx = (static_cast<float>(x) + 0.5f) / box.scale - 0.5f;
            float b, g, r;
            sampleBilinear(src, w, h, sx, sy, &b, &g, &r);
            const int dx = x0 + x;
            const int dy = y0 + y;
            if (dx < 0 || dy < 0 || dx >= side || dy >= side) {
                continue;
            }
            const size_t p = static_cast<size_t>(dy) * side + dx;
            const size_t plane = static_cast<size_t>(side) * side;
            box.bgr[p] = b;
            box.bgr[plane + p] = g;
            box.bgr[2 * plane + p] = r;
        }
    }
    return box;
}

void bilinearResizeBgr(const std::uint8_t* src, int sw, int sh,
                         std::uint8_t* dst, int dw, int dh) {
    for (int dy = 0; dy < dh; ++dy) {
        const float sy = (static_cast<float>(dy) + 0.5f) * sh / dh - 0.5f;
        for (int dx = 0; dx < dw; ++dx) {
            const float sx = (static_cast<float>(dx) + 0.5f) * sw / dw - 0.5f;
            float b, g, r;
            sampleBilinear(src, sw, sh, sx, sy, &b, &g, &r);
            std::uint8_t* out = dst + (static_cast<size_t>(dy) * dw + dx) * 3;
            out[0] = static_cast<std::uint8_t>(b * 255.f + 0.5f);
            out[1] = static_cast<std::uint8_t>(g * 255.f + 0.5f);
            out[2] = static_cast<std::uint8_t>(r * 255.f + 0.5f);
        }
    }
}

void areaResizeBgr(const std::uint8_t* src, int sw, int sh,
                    std::uint8_t* dst, int dw, int dh) {
    const double sx = static_cast<double>(sw) / dw;
    const double sy = static_cast<double>(sh) / dh;
    for (int dy = 0; dy < dh; ++dy) {
        int y0 = static_cast<int>(dy * sy);
        int y1 = static_cast<int>((dy + 1) * sy);
        if (y1 <= y0) y1 = y0 + 1;
        if (y1 > sh) y1 = sh;
        for (int dx = 0; dx < dw; ++dx) {
            int x0 = static_cast<int>(dx * sx);
            int x1 = static_cast<int>((dx + 1) * sx);
            if (x1 <= x0) x1 = x0 + 1;
            if (x1 > sw) x1 = sw;
            float b = 0.f, g = 0.f, r = 0.f;
            for (int y = y0; y < y1; ++y) {
                const std::uint8_t* row = src + (static_cast<size_t>(y) * sw + x0) * 3;
                for (int x = x0; x < x1; ++x) {
                    b += row[0];
                    g += row[1];
                    r += row[2];
                    row += 3;
                }
            }
            const float inv = 1.f / static_cast<float>((x1 - x0) * (y1 - y0));
            std::uint8_t* out = dst + (static_cast<size_t>(dy) * dw + dx) * 3;
            out[0] = static_cast<std::uint8_t>(b * inv + 0.5f);
            out[1] = static_cast<std::uint8_t>(g * inv + 0.5f);
            out[2] = static_cast<std::uint8_t>(r * inv + 0.5f);
        }
    }
}

}  // namespace

struct BirdEngine::Impl {
#ifdef BIRDY_HAS_NCNN
    ncnn::Net detector;
    ncnn::Net classifier;
#endif
};

BirdEngine::BirdEngine() : impl_(std::make_unique<Impl>()) {}
BirdEngine::~BirdEngine() = default;
BirdEngine::BirdEngine(BirdEngine&&) noexcept = default;
BirdEngine& BirdEngine::operator=(BirdEngine&&) noexcept = default;

bool BirdEngine::inferenceLinked() const {
#ifdef BIRDY_HAS_NCNN
    return true;
#else
    return false;
#endif
}

const SpeciesName* BirdEngine::speciesAt(int index) const {
    if (index < 0 || index >= static_cast<int>(species_.size())) {
        return nullptr;
    }
    return &species_[index];
}

std::string BirdEngine::status() const {
    std::ostringstream os;
    os << "名录 " << species_.size() << " 种";
    if (!inferenceLinked()) {
        os << "；推理未链接 ncnn";
    } else if (detectorReady_ && classifierReady_) {
        os << "；鸟检与认种已加载";
    } else {
        os << "；" << backendNote_;
    }
    return os.str();
}

bool BirdEngine::load(const std::string& modelDir, std::string* error) {
    modelDir_ = modelDir;
    infoReady_ = false;
    detectorReady_ = false;
    classifierReady_ = false;
    backendNote_.clear();
    const std::string info = modelDir + "/bird_info.json";
    if (!parseBirdInfo(info, &species_)) {
        if (error) {
            *error = "无法读取名录：" + info;
        }
        return false;
    }
    infoReady_ = true;
#ifndef BIRDY_HAS_NCNN
    backendNote_ = "当前构建没有链接 ncnn";
    if (error) {
        error->clear();
    }
    return true;
#else
    auto join = [&](const char* name) { return modelDir + "/" + name; };
    impl_->detector.opt.use_vulkan_compute = false;
    impl_->detector.opt.num_threads = 4;
    impl_->detector.opt.lightmode = true;
    const std::string detP = join("bird-seg.param");
    const std::string detB = join("bird-seg.bin");
    if (impl_->detector.load_param(detP.c_str()) != 0 ||
        impl_->detector.load_model(detB.c_str()) != 0) {
        backendNote_ = "鸟检模型未加载";
        if (error) {
            *error = "无法加载 " + detP;
        }
        return false;
    }
    detectorReady_ = true;
    impl_->classifier.opt.use_vulkan_compute = false;
    impl_->classifier.opt.num_threads = 4;
    impl_->classifier.opt.lightmode = true;
    const std::string clsP = join("bird-iden.param");
    const std::string clsB = join("bird-iden.bin");
    if (impl_->classifier.load_param(clsP.c_str()) != 0 ||
        impl_->classifier.load_model(clsB.c_str()) != 0) {
        backendNote_ = "认种模型未加载";
        if (error) {
            *error = "无法加载 " + clsP;
        }
        return false;
    }
    classifierReady_ = true;
    backendNote_ = "ncnn";
    if (error) {
        error->clear();
    }
    return true;
#endif
}

bool BirdEngine::detectAndClassify(const std::uint8_t* bgr, int width, int height,
                                   float conf, float nms,
                                   std::vector<Detection>* out, std::string* error) {
    if (!out) {
        return false;
    }
    out->clear();
    if (!bgr || width < 8 || height < 8) {
        if (error) {
            *error = "图像无效";
        }
        return false;
    }
#ifndef BIRDY_HAS_NCNN
    if (error) {
        *error = "当前构建没有链接 ncnn";
    }
    return false;
#else
    if (!detectorReady_ || !classifierReady_) {
        if (error) {
            *error = "模型尚未加载";
        }
        return false;
    }
    const Letterbox box = makeLetterbox(bgr, width, height, kDetSize);
    ncnn::Mat in;
    in.create(kDetSize, kDetSize, 3);
    const size_t plane = static_cast<size_t>(kDetSize) * kDetSize;
    for (int c = 0; c < 3; ++c) {
        std::memcpy(in.channel(c).data, box.bgr.data() + c * plane, plane * sizeof(float));
    }
    ncnn::Extractor ex = impl_->detector.create_extractor();
    ex.input("in0", in);
    ncnn::Mat raw;
    if (ex.extract("out0", raw) != 0) {
        if (error) {
            *error = "鸟检推理失败";
        }
        return false;
    }
    const int needCh = 4 + kCocoClasses;
    const int segCh = needCh + 32;
    int layout = 2;
    if (raw.c >= needCh) {
        layout = 0;
    } else if (raw.h == needCh || raw.h == segCh) {
        layout = 1;
    }
    const int anchors = layout == 0 ? raw.h : (layout == 1 ? raw.w : raw.h);
    auto chval = [&](int c, int a) -> float {
        if (layout == 0) return raw.channel(c).row(0)[a];
        if (layout == 1) return raw.channel(0).row(c)[a];
        return raw.channel(0).row(a)[c];
    };
    std::vector<Detection> dets;
    const int scoreCh = 4 + kBirdClass;
    for (int a = 0; a < anchors; ++a) {
        const float score = chval(scoreCh, a);
        if (score < conf) {
            continue;
        }
        const float cx = chval(0, a);
        const float cy = chval(1, a);
        const float bw = chval(2, a);
        const float bh = chval(3, a);
        Detection d;
        d.x1 = (cx - bw * 0.5f - box.padW) / box.scale;
        d.y1 = (cy - bh * 0.5f - box.padH) / box.scale;
        d.x2 = (cx + bw * 0.5f - box.padW) / box.scale;
        d.y2 = (cy + bh * 0.5f - box.padH) / box.scale;
        d.x1 = std::max(0.f, std::min(static_cast<float>(width), d.x1));
        d.y1 = std::max(0.f, std::min(static_cast<float>(height), d.y1));
        d.x2 = std::max(0.f, std::min(static_cast<float>(width), d.x2));
        d.y2 = std::max(0.f, std::min(static_cast<float>(height), d.y2));
        if (d.x2 - d.x1 < 4.f || d.y2 - d.y1 < 4.f) {
            continue;
        }
        d.detScore = score;
        dets.push_back(d);
    }
    *out = runNms(std::move(dets), nms);

    for (Detection& d : *out) {
        int x1 = std::max(0, static_cast<int>(d.x1) - kCropMargin);
        int y1 = std::max(0, static_cast<int>(d.y1) - kCropMargin);
        int x2 = std::min(width, static_cast<int>(std::ceil(d.x2)) + kCropMargin);
        int y2 = std::min(height, static_cast<int>(std::ceil(d.y2)) + kCropMargin);
        const int rw = x2 - x1;
        const int rh = y2 - y1;
        if (rw < 8 || rh < 8) {
            continue;
        }
        std::vector<std::uint8_t> crop(static_cast<size_t>(rw) * rh * 3);
        for (int y = 0; y < rh; ++y) {
            std::memcpy(crop.data() + static_cast<size_t>(y) * rw * 3,
                        bgr + (static_cast<size_t>(y1 + y) * width + x1) * 3,
                        static_cast<size_t>(rw) * 3);
        }
        const float rs = static_cast<float>(kClsShort) / std::min(rw, rh);
        const int rw2 = std::max(kClsShort, static_cast<int>(std::lround(rw * rs)));
        const int rh2 = std::max(kClsShort, static_cast<int>(std::lround(rh * rs)));
        std::vector<std::uint8_t> resized(static_cast<size_t>(rw2) * rh2 * 3);
        if (rw > rw2 || rh > rh2) {
            areaResizeBgr(crop.data(), rw, rh, resized.data(), rw2, rh2);
        } else {
            bilinearResizeBgr(crop.data(), rw, rh, resized.data(), rw2, rh2);
        }
        const int offX = (rw2 - kClsSize) / 2;
        const int offY = (rh2 - kClsSize) / 2;
        std::vector<float> chw(static_cast<size_t>(3) * kClsSize * kClsSize);
        for (int dy = 0; dy < kClsSize; ++dy) {
            const std::uint8_t* row =
                resized.data() + (static_cast<size_t>(offY + dy) * rw2 + offX) * 3;
            for (int dx = 0; dx < kClsSize; ++dx) {
                const unsigned char pb = row[dx * 3 + 0];
                const unsigned char pg = row[dx * 3 + 1];
                const unsigned char pr = row[dx * 3 + 2];
                const unsigned char rgb[3] = {pr, pg, pb};
                const size_t pi = static_cast<size_t>(dy) * kClsSize + dx;
                for (int c = 0; c < 3; ++c) {
                    chw[static_cast<size_t>(c) * kClsSize * kClsSize + pi] =
                        (rgb[c] / 255.f - kBgrMean[2 - c]) / kBgrStd[2 - c];
                }
            }
        }
        ncnn::Mat cin;
        cin.create(kClsSize, kClsSize, 3);
        const size_t cplane = static_cast<size_t>(kClsSize) * kClsSize;
        for (int c = 0; c < 3; ++c) {
            std::memcpy(cin.channel(c).data, chw.data() + c * cplane, cplane * sizeof(float));
        }
        ncnn::Extractor cex = impl_->classifier.create_extractor();
        cex.input("in0", cin);
        ncnn::Mat cout;
        if (cex.extract("out0", cout) != 0 || cout.data == nullptr) {
            continue;
        }
        int nc = cout.w * cout.h * cout.c;
        if (!species_.empty() && static_cast<int>(species_.size()) < nc) {
            nc = static_cast<int>(species_.size());
        }
        if (nc <= 1) {
            continue;
        }
        const float* logits = static_cast<const float*>(cout.data);
        int best = 0;
        float bv = logits[0];
        for (int i = 1; i < nc; ++i) {
            if (logits[i] > bv) {
                bv = logits[i];
                best = i;
            }
        }
        double sum = 0.0;
        for (int i = 0; i < nc; ++i) {
            sum += std::exp(static_cast<double>(logits[i] - bv));
        }
        d.clsIndex = best;
        d.clsScore = static_cast<float>(1.0 / sum);
    }
    if (error) {
        error->clear();
    }
    return true;
#endif
}

}  // namespace birdy
