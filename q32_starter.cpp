// Requires: stb_image.h, stb_image_write.h in project folder
#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <random>
#include <string>

// ---------- HELPER FUNCTIOS BELOW (DO NOT MODIFY) ----------
template <class T> T clampv(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Mirror padding function
int reflect_index(int i, int n) {
    if (n <= 1) return 0;
    while (i < 0 || i >= n) { i = (i < 0) ? -i : (2 * n - 2 - i); }
    return i;
}

// Computing PSNR
double psnr8(const std::vector<uint8_t>& gt, const std::vector<uint8_t>& test) {
    assert(gt.size() == test.size());
    double acc{ 0.0 };
    for (std::size_t i{ 0 }; i < gt.size(); ++i) {
        double d{ double(gt[i]) - double(test[i]) };
        acc += d * d;
    }
    double m{ acc / double(gt.size()) };
    if (m == 0.0) return 99.0;
    return 10.0 * std::log10((255.0 * 255.0) / m);
}

// Saving PNG image file
void save_png(const std::string& fname,
    const std::vector<uint8_t>& img,
    int H, int W) {
    stbi_write_png(fname.c_str(), W, H, 1, img.data(), W);
}


// YOUR MAIN (PLEASE FILL IN TODO's FROM HERE)
int main() {
    // Loading JPEG as grayscale and center-crop 256x256
    int W_in, H_in, ch;
    uint8_t* data = stbi_load("ground_truth.jpg", &W_in, &H_in, &ch, 1);
    if (!data) { std::fprintf(stderr, "ERROR: cannot load ground_truth.jpg\n"); return 1; }
    if (W_in < 256 || H_in < 256) {
        std::fprintf(stderr, "ERROR: need at least 256x256, got %dx%d\n", W_in, H_in);
        stbi_image_free(data); return 1;
    }

    const int H{ 256 }, W{ 256 };
    const int r0{ H_in / 2 - H / 2 }, c0{ W_in / 2 - W / 2 };
    std::vector<uint8_t> I_gt(H * W, 0);
    for (int r{ 0 }; r < H; ++r) {
        const uint8_t* src{ data + (r0 + r) * W_in };
        uint8_t* dst{ I_gt.data() + r * W };
        std::copy(src + c0, src + c0 + W, dst);
    }
    stbi_image_free(data);
    save_png("gt.png", I_gt, H, W);

     // Generating noisy signal: Gaussian noise + salt/pepper
     std::vector<uint8_t> I_noisy{ I_gt };
     {
         std::mt19937 rng{ 4242u };
         std::normal_distribution<double> gaus{ 0.0, 18.0 };
         std::uniform_real_distribution<double> uni{ 0.0, 1.0 };

         // Gaussian noise
         for (std::size_t i{ 0 }; i < I_noisy.size(); ++i) {
             const uint8_t& src{ I_noisy[i] };
             double v{ double(src) + gaus(rng) };
             uint8_t& dst{ I_noisy[i] };
             dst = static_cast<uint8_t>(clampv<int>(int(std::lround(v)), 0, 255));
         }
         // Salt & pepper noise
         for (std::size_t i{ 0 }; i < I_noisy.size(); ++i) {
             double p{ uni(rng) };
             if (p < 0.01) { uint8_t& px{ I_noisy[i] }; px = 0; }
             else if (p > 0.99) { uint8_t& px{ I_noisy[i] }; px = 255; }
         }
     }
    save_png("noisy.png", I_noisy, H, W);

    std::vector<uint8_t> I_blur_h{ I_noisy }, I_blur{ I_noisy };

    // --- Horizontal pass for Gaussian blur ---
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            int c0{ reflect_index(c - 1, W) };
            int c1{ c };
            int c2{ reflect_index(c + 1, W) };

            const uint8_t& a{ I_noisy[r * W + c0] };
            const uint8_t& b{ I_noisy[r * W + c1] };
            const uint8_t& c_val{ I_noisy[r * W + c2] };

            double val = (static_cast<double>(a) + static_cast<double>(b) * 2.0 + static_cast<double>(c_val)) / 4.0;
            I_blur_h[r * W + c] = static_cast<uint8_t>(clampv<int>(static_cast<int>(std::lround(val)), 0, 255));
        }
    }

    // --- Vertical pass for Gaussian blur ---
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            int r0{ reflect_index(r - 1, H) };
            int r1{ r };
            int r2{ reflect_index(r + 1, H) };
            
            const uint8_t& a{ I_blur_h[r0 * W + c] };
            const uint8_t& b{ I_blur_h[r1 * W + c] };
            const uint8_t& c_val{ I_blur_h[r2 * W + c] };

            double val = (static_cast<double>(a) + static_cast<double>(b) * 2.0 + static_cast<double>(c_val)) / 4.0;
            I_blur[r * W + c] = static_cast<uint8_t>(clampv<int>(static_cast<int>(std::lround(val)), 0, 255));
        }
    }
    save_png("blur.png", I_blur, H, W);

    // --- 3x3 median filter ---
    std::vector<uint8_t> I_med{ I_noisy };
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            int r0{ reflect_index(r - 1, H) }, r1{ r }, r2{ reflect_index(r + 1, H) };
            int c0{ reflect_index(c - 1, W) }, c1{ c }, c2{ reflect_index(c + 1, W) };

            const uint8_t& p00{ I_noisy[r0 * W + c0] };
            const uint8_t& p01{ I_noisy[r0 * W + c1] };
            const uint8_t& p02{ I_noisy[r0 * W + c2] };
            const uint8_t& p10{ I_noisy[r1 * W + c0] };
            const uint8_t& p11{ I_noisy[r1 * W + c1] };
            const uint8_t& p12{ I_noisy[r1 * W + c2] };
            const uint8_t& p20{ I_noisy[r2 * W + c0] };
            const uint8_t& p21{ I_noisy[r2 * W + c1] };
            const uint8_t& p22{ I_noisy[r2 * W + c2] };

            std::array<uint8_t, 9> window{ p00, p01, p02, p10, p11, p12, p20, p21, p22 };
            std::sort(window.begin(), window.end());
            I_med[r * W + c] = window[4];
        }
    }
    save_png("median.png", I_med, H, W);


    // ---- Report PSNRs ----
    std::printf("PSNR (dB) vs gt:\n");
    std::printf("  noisy : %.2f\n", psnr8(I_gt, I_noisy));
    std::printf("  blur  : %.2f\n", psnr8(I_gt, I_blur));
    std::printf("  median: %.2f\n", psnr8(I_gt, I_med));

    std::puts("Saved: gt.png, noisy.png, blur.png, median.png");
    return 0;
}