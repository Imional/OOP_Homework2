// Requires: stb_image_write.h in project folder
#define _CRT_SECURE_NO_WARNINGS
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

// Computing mean-squared error
double mse(const std::vector<double>& a, const std::vector<double>& b) {
    assert(a.size() == b.size());
    double acc{ 0.0 };
    for (std::size_t i{ 0 }; i < a.size(); ++i) { double d{ a[i] - b[i] }; acc += d * d; }
    return acc / static_cast<double>(a.size());
}

static inline void set_px(std::vector<uint8_t>& im, int W, int H, int x, int y,
    uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    int idx{ (y * W + x) * 3 }; im[idx] = r; im[idx + 1] = g; im[idx + 2] = b;
}
void draw_line(std::vector<uint8_t>& im, int W, int H,
    int x0, int y0, int x1, int y1,
    uint8_t r, uint8_t g, uint8_t b) {
    int dx{ std::abs(x1 - x0) }, sx{ x0 < x1 ? 1 : -1 };
    int dy{ -std::abs(y1 - y0) }, sy{ y0 < y1 ? 1 : -1 };
    int err{ dx + dy }, e2;
    while (true) {
        set_px(im, W, H, x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void fill_rect(std::vector<uint8_t>& im, int W, int H, int x0, int y0, int w, int h,
    uint8_t r, uint8_t g, uint8_t b) {
    for (int y{ 0 }; y < h; ++y) for (int x{ 0 }; x < w; ++x) set_px(im, W, H, x0 + x, y0 + y, r, g, b);
}
void draw_char5x7(std::vector<uint8_t>& im, int W, int H, int x, int y, char ch,
    uint8_t r, uint8_t g, uint8_t b) {
    const uint8_t* G{ nullptr };
    static const uint8_t Cg[7]{ 0x0E,0x11,0x10,0x10,0x10,0x11,0x0E };
    static const uint8_t Ng[7]{ 0x11,0x19,0x15,0x13,0x11,0x11,0x11 };
    static const uint8_t Bg[7]{ 0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E };
    static const uint8_t Wg[7]{ 0x11,0x11,0x11,0x15,0x15,0x15,0x0A };
    static const uint8_t Mg[7]{ 0x11,0x1B,0x15,0x15,0x11,0x11,0x11 };
    switch (ch) {
    case 'C': G = Cg; break; case 'N': G = Ng; break; case 'B': G = Bg; break;
    case 'W': G = Wg; break; case 'M': G = Mg; break; default: return;
    }
    for (int row{ 0 }; row < 7; ++row)
        for (int col{ 0 }; col < 5; ++col)
            if (G[row] & (1u << (4 - col))) set_px(im, W, H, x + col, y + row, r, g, b);
}
void plot_signals_png(const std::string& fname, int H_img, int sx,
    const std::vector<double>& clean,
    const std::vector<double>& noisy,
    const std::vector<double>& box,
    const std::vector<double>& wavg,
    const std::vector<double>& med) {
    int N{ int(clean.size()) };
    int L{ 40 }, R{ 10 }, T{ 10 }, B{ 20 };
    int W{ L + R + N * sx };
    std::vector<uint8_t> rgb(std::size_t(W * H_img * 3), 255); // white

    for (int x{ 0 }; x < W; ++x) set_px(rgb, W, H_img, x, H_img - B, 200, 200, 200);
    for (int y{ 0 }; y < H_img; ++y) set_px(rgb, W, H_img, L, y, 200, 200, 200);

    auto vmin = [](const auto& v) { return *std::min_element(v.begin(), v.end()); };
    auto vmax = [](const auto& v) { return *std::max_element(v.begin(), v.end()); };
    double mn{ std::min({vmin(clean),vmin(noisy),vmin(box),vmin(wavg),vmin(med)}) };
    double mx{ std::max({vmax(clean),vmax(noisy),vmax(box),vmax(wavg),vmax(med)}) };
    if (mx <= mn) mx = mn + 1.0;

    auto Y = [&](double v)->int {
        double t = (v - mn) / (mx - mn);
        t = clampv(t, 0.0, 1.0);
        return T + int((1.0 - t) * (H_img - T - B));
        };
    auto X = [&](int i)->int { return L + i * sx; };
    auto draw_series = [&](const std::vector<double>& s, uint8_t r, uint8_t g, uint8_t b) {
        int x0{ X(0) }, y0{ Y(s[0]) };
        for (int i{ 1 }; i < N; ++i) {
            int x1{ X(i) }, y1{ Y(s[i]) };
            draw_line(rgb, W, H_img, x0, y0, x1, y1, r, g, b); x0 = x1; y0 = y1;
        }
        };

    draw_series(clean, 0, 0, 0);
    draw_series(noisy, 220, 0, 0);
    draw_series(box, 0, 160, 0);
    draw_series(wavg, 0, 0, 220);
    draw_series(med, 200, 0, 200);

    // Legend (top-right)
    struct Entry { char ch; uint8_t r, g, b; };
    std::array<Entry, 5> Lg{ Entry{'C',0,0,0}, Entry{'N',220,0,0}, Entry{'B',0,160,0},
                            Entry{'W',0,0,220}, Entry{'M',200,0,200} };
    int pad{ 6 }, sw{ 18 }, lh{ 12 }, tw{ 5 }, th{ 7 };
    int lw{ pad * 2 + sw + 6 + tw }, lhbox{ pad * 2 + int(Lg.size()) * lh };
    int lx{ W - (10 + lw) }, ly{ T + 6 };
    fill_rect(rgb, W, H_img, lx, ly, lw, lhbox, 255, 255, 255);
    for (int x{ 0 }; x < lw; ++x) {
        set_px(rgb, W, H_img, lx + x, ly, 180, 180, 180);
        set_px(rgb, W, H_img, lx + x, ly + lhbox - 1, 180, 180, 180);
    }
    for (int y{ 0 }; y < lhbox; ++y) {
        set_px(rgb, W, H_img, lx, ly + y, 180, 180, 180);
        set_px(rgb, W, H_img, lx + lw - 1, ly + y, 180, 180, 180);
    }
    for (int i{ 0 }; i<int(Lg.size()); ++i) {
        int ymid{ ly + pad + i * lh + lh / 2 }; int x0{ lx + pad }, x1{ x0 + sw };
        draw_line(rgb, W, H_img, x0, ymid, x1, ymid, Lg[i].r, Lg[i].g, Lg[i].b);
        draw_char5x7(rgb, W, H_img, x1 + 6, ymid - th / 2, Lg[i].ch, 0, 0, 0);
    }

    stbi_write_png(fname.c_str(), W, H_img, 3, rgb.data(), W * 3);
}


// YOUR MAIN (PLEASE FILL IN TODO's FROM HERE)
#ifdef __cpp_lib_math_constants
#include <numbers>
constexpr double pi{ std::numbers::pi_v<double> };
#else
constexpr double pi{ 3.14159265358979323846 };
#endif

int main() {
    const std::size_t N{ 257 };

    // Ground-truth signal: trend + two tones
    std::vector<double> x_clean(N, 0.0);
    {
        const double f1{ 7.0 }, f2{ 23.0 };
        for (std::size_t n{ 0 }; n < N; ++n) {
            double t{ double(n) / double(N) };
            x_clean[n] = 0.15 * t + 0.6 * std::sin(2.0 * pi * f1 * t) + 0.3 * std::sin(2.0 * pi * f2 * t);
        }
    }

    // Noisy signal (Gaussian noise + impulses + clipping)
    std::vector<double> x_noisy{ x_clean };
    {
        std::mt19937 rng{ 12345u };
        std::normal_distribution<double> gaus{ 0.0, 0.15 };
        for (double& v : x_noisy) { v += gaus(rng); }
        for (int k{ 0 }; k < 6; ++k) {
            std::size_t i{ std::size_t((k + 1) * N / 7) };
            x_noisy[i] += (k % 2 ? 2.5 : -2.5);
        }
        for (double& v : x_noisy) v = clampv(v, -1.1, 1.1);
    }

    // TODO: implement denoisers using window **const references**
    std::vector<double> y_sma{ x_noisy }, y_wma{ x_noisy }, y_med{ x_noisy };

    // --- Simple Moving Average ---
    for (size_t i = 0; i < N; ++i) {
        int i0{ reflect_index(static_cast<int>(i) - 1, static_cast<int>(N)) };
        int i1{ static_cast<int>(i) };
        int i2{ reflect_index(static_cast<int>(i) + 1, static_cast<int>(N)) };
        
        const double& a{ x_noisy[i0] };
        const double& b{ x_noisy[i1] };
        const double& c{ x_noisy[i2] };
        
        y_sma[i] = (a + b + c) / 3.0;
    }

    // --- Weighted Moving Average (use weight function: [1 2 1]/4) ---
    for (size_t i = 0; i < N; ++i) {
        int i0{ reflect_index(static_cast<int>(i) - 1, static_cast<int>(N)) };
        int i1{ static_cast<int>(i) };
        int i2{ reflect_index(static_cast<int>(i) + 1, static_cast<int>(N)) };

        const double& a{ x_noisy[i0] };
        const double& b{ x_noisy[i1] };
        const double& c{ x_noisy[i2] };
        
        y_wma[i] = (a * 1.0 + b * 2.0 + c * 1.0) / 4.0;
    }

    // --- Median of three ---
    for (size_t i = 0; i < N; ++i) {
        int i0{ reflect_index(static_cast<int>(i) - 1, static_cast<int>(N)) };
        int i1{ static_cast<int>(i) };
        int i2{ reflect_index(static_cast<int>(i) + 1, static_cast<int>(N)) };

        const double& a{ x_noisy[i0] };
        const double& b{ x_noisy[i1] };
        const double& c{ x_noisy[i2] };
        
        std::array<double, 3> window {a, b, c};
        std::sort(window.begin(), window.end());
        y_med[i] = window[1];
    }


    // Printing output text
    std::printf("MSE vs clean:\n");
    std::printf("  noisy : %.6f\n", mse(x_clean, x_noisy));
    std::printf("  box   : %.6f\n", mse(x_clean, y_sma));
    std::printf("  wavg  : %.6f\n", mse(x_clean, y_wma));
    std::printf("  median: %.6f\n", mse(x_clean, y_med));

    // Generating a PNG file with line profiles
    plot_signals_png("signals.png", /*H_img=*/320, /*sx=*/3,
        x_clean, x_noisy, y_sma, y_wma, y_med);
    std::puts("Saved: signals.png (C=clean, N=noisy, B=box(SMA), W=wma, M=median)");
    return 0;
}