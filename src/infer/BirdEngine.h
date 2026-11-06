#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace birdy {

struct SpeciesName {
    std::string cn;
    std::string en;
    std::string latin;
};

struct Detection {
    float x1 = 0.f;
    float y1 = 0.f;
    float x2 = 0.f;
    float y2 = 0.f;
    float detScore = 0.f;
    float clsScore = 0.f;
    int clsIndex = -1;
};

// 桌面端鸟检与认种。算法与 birdy/app 里已验证的 ncnn 路径一致：
// 640 letterbox 的 YOLOv8-seg（鸟 = COCO 类 14），认种为短边 256、中心裁 224。
class BirdEngine {
public:
    BirdEngine();
    ~BirdEngine();
    BirdEngine(BirdEngine&&) noexcept;
    BirdEngine& operator=(BirdEngine&&) noexcept;
    BirdEngine(const BirdEngine&) = delete;
    BirdEngine& operator=(const BirdEngine&) = delete;

    bool load(const std::string& modelDir, std::string* error);
    bool ready() const { return infoReady_; }
    bool inferenceLinked() const;
    int speciesCount() const { return static_cast<int>(species_.size()); }
    const SpeciesName* speciesAt(int index) const;
    std::string status() const;

    // bgr 为连续 8 位 BGR，行宽等于 width。未链接 ncnn 或模型未加载时返回 false。
    bool detectAndClassify(const std::uint8_t* bgr, int width, int height,
                           float conf, float nms,
                           std::vector<Detection>* out, std::string* error);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string modelDir_;
    std::vector<SpeciesName> species_;
    bool infoReady_ = false;
    bool detectorReady_ = false;
    bool classifierReady_ = false;
    std::string backendNote_;
};

}  // namespace birdy
