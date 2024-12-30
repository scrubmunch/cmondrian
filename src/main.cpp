#include "termbox.h"
#include <iostream>
#include <random>
#include <cstdlib>
#include <vector>
#include <algorithm>

const int MARGIN_PERCENT = 10;
const int VARIATION_FRACTION = 3;  // 1/3 variation in line positions
const float MERGE_FRACTION = 0.75; // Try to merge 75% of rectangles

struct Color {
    uint32_t ch;      // Unicode character for different densities
    uint16_t fg;      // Foreground color
    uint16_t bg;      // Background color
};

struct line {
    int x1;
    int y1;
    int x2;
    int y2;
};

struct Rectangle {
    int x1, y1;
    int x2, y2;
    Color color;
};

// Colors using different block characters
const Color TRANSPARENT = {' ', TB_DEFAULT, TB_DEFAULT};
const Color TRANSPARENT_RED = {0x2593, TB_RED, TB_DEFAULT};     // Transparent ▓
const Color TRANSPARENT_BLUE = {0x2593, TB_BLUE, TB_DEFAULT};
const Color TRANSPARENT_YELLOW = {0x2593, TB_YELLOW, TB_DEFAULT};
const Color TRANSPARENT_WHITE = {0x2593, TB_WHITE, TB_DEFAULT};
const Color OPAQUE_RED = {0x2588, TB_RED, TB_DEFAULT};    // Opaque █
const Color OPAQUE_BLUE = {0x2588, TB_BLUE, TB_DEFAULT};
const Color OPAQUE_YELLOW = {0x2588, TB_YELLOW, TB_DEFAULT};
const Color OPAQUE_WHITE = {0x2588, TB_WHITE, TB_DEFAULT};

const std::vector<Color> OPAQUE_PALETTE = {
    OPAQUE_RED,
    OPAQUE_BLUE,
    OPAQUE_YELLOW,
    OPAQUE_WHITE, // Multiple white values to increase probability
    OPAQUE_WHITE,
    OPAQUE_WHITE,
};

const std::vector<Color> TRANSPARENT_PALETTE = {
    TRANSPARENT_RED,
    TRANSPARENT_BLUE,
    TRANSPARENT_YELLOW,
    TRANSPARENT_WHITE, // Multiple white values to increase probability
    TRANSPARENT_WHITE,
    TRANSPARENT_WHITE,
};

int random_value(int min, int max){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

void color_rectangles(std::vector<Rectangle>& rectangles, const std::vector<Color>& palette) {
    for (auto& rect : rectangles) {
        rect.color = palette[random_value(0, palette.size() - 1)];
    }
}

void draw_rectangle(const Rectangle& rect) {
    if (rect.color.ch != ' ') {  // If not transparent
        for (int y = rect.y1 + 1; y < rect.y2; y++) {
            for (int x = rect.x1 + 1; x < rect.x2; x++) {
                tb_change_cell(x, y, rect.color.ch, rect.color.fg, rect.color.bg);
            }
        }
    }

    // Draw the borders
    // Horizontal lines
    for (int x = rect.x1; x <= rect.x2; x++) {
        tb_change_cell(x, rect.y1, ' ', TB_DEFAULT, TB_DEFAULT);
        tb_change_cell(x, rect.y2, ' ', TB_DEFAULT, TB_DEFAULT);
    }
    // Vertical lines
    for (int y = rect.y1; y <= rect.y2; y++) {
        tb_change_cell(rect.x1, y, ' ', TB_DEFAULT, TB_DEFAULT);
        tb_change_cell(rect.x2, y, ' ', TB_DEFAULT, TB_DEFAULT);
        // Extra lines to get similar vertical thickness
        tb_change_cell(rect.x1 + 1, y, ' ', TB_DEFAULT, TB_DEFAULT);
        tb_change_cell(rect.x2 + 1, y, ' ', TB_DEFAULT, TB_DEFAULT);
    }
}

bool are_adjacent(const Rectangle& r1, const Rectangle& r2) {
    // Check if rectangles share a vertical edge
    bool vertical_adjacent = (r1.x2 == r2.x1 || r1.x1 == r2.x2) &&
                           (r1.y1 == r2.y1 && r1.y2 == r2.y2);

    // Check if rectangles share a horizontal edge
    bool horizontal_adjacent = (r1.y2 == r2.y1 || r1.y1 == r2.y2) &&
                             (r1.x1 == r2.x1 && r1.x2 == r2.x2);

    return vertical_adjacent || horizontal_adjacent;
}

std::vector<Rectangle> find_rectangles(const std::vector<line>& vertical_lines,
                                     const std::vector<line>& horizontal_lines) {
    std::vector<int> x_coords;
    std::vector<int> y_coords;

    // Collect all x coordinates
    x_coords.push_back(0);
    for (const auto& line : vertical_lines) {
        x_coords.push_back(line.x1);
    }
    x_coords.push_back(tb_width()-1);

    // Collect all y coordinates
    y_coords.push_back(0);
    for (const auto& line : horizontal_lines) {
        y_coords.push_back(line.y1);
    }
    y_coords.push_back(tb_height()-1);

    // Sort coordinates
    std::sort(x_coords.begin(), x_coords.end());
    std::sort(y_coords.begin(), y_coords.end());

    // Create rectangles from grid
    std::vector<Rectangle> rectangles;
    for (size_t i = 0; i < x_coords.size() - 1; i++) {
        for (size_t j = 0; j < y_coords.size() - 1; j++) {
            rectangles.push_back({
                x_coords[i], y_coords[j],
                x_coords[i+1], y_coords[j+1],
                TRANSPARENT
            });
        }
    }

    return rectangles;
}

void merge_rectangles(std::vector<Rectangle>& rectangles) {
    int merge_attempts = rectangles.size() * MERGE_FRACTION;

    for (int i = 0; i < merge_attempts; i++) {
        if (rectangles.size() < 2) break; // Stop if we can't merge anymore

        // Randomly select a rectangle
        int idx1 = random_value(0, rectangles.size() - 1);

        // Find all adjacent rectangles
        std::vector<int> adjacent_indices;
        for (int j = 0; j < rectangles.size(); j++) {
            if (j != idx1 && are_adjacent(rectangles[idx1], rectangles[j])) {
                adjacent_indices.push_back(j);
            }
        }

        // If we found adjacent rectangles
        if (!adjacent_indices.empty()) {
            // Randomly select one adjacent rectangle
            int adj_idx = adjacent_indices[random_value(0, adjacent_indices.size() - 1)];

            // Create new merged rectangle
            Rectangle merged = {
                std::min(rectangles[idx1].x1, rectangles[adj_idx].x1),
                std::min(rectangles[idx1].y1, rectangles[adj_idx].y1),
                std::max(rectangles[idx1].x2, rectangles[adj_idx].x2),
                std::max(rectangles[idx1].y2, rectangles[adj_idx].y2),
                TRANSPARENT
            };

            // Replace the first rectangle with merged one
            rectangles[idx1] = merged;
            // Remove the second rectangle
            rectangles.erase(rectangles.begin() + adj_idx);
        }
    }
}

// Calculate number of lines based on terminal dimensions
void calculate_line_counts(int width, int height, int& vert_lines, int& horiz_lines, int density = 8) {
    // Calculate aspect ratio
    float aspect_ratio = static_cast<float>(width) / height;

    // Base number of lines on the smaller dimension
    // density: lower number = more lines, higher number = fewer lines
    int base_lines = std::min(width, height) / density;

    // Adjust line counts based on aspect ratio
    if (aspect_ratio > 1) { // Wider than tall
        vert_lines = static_cast<int>(base_lines * aspect_ratio);
        horiz_lines = base_lines;
    } else { // Taller than wide
        vert_lines = base_lines;
        horiz_lines = static_cast<int>(base_lines / aspect_ratio);
    }

    // Ensure minimum and maximum values
    vert_lines = std::max(2, std::min(vert_lines, 8));
    horiz_lines = std::max(2, std::min(horiz_lines, 8));
}

// Generate evenly spaced positions with some randomness
std::vector<int> generate_positions(int start, int end, int count) {
    std::vector<int> positions;
    positions.push_back(start); // Always include start position

    int margin = (end - start) / MARGIN_PERCENT;
    int usable_space = end - start - (2 * margin);
    int segment = usable_space / (count + 1);

    for (int i = 1; i <= count; i++) {
        int base_pos = start + margin + (i * segment);
        int variation = segment / VARIATION_FRACTION;
        int pos = base_pos + random_value(-variation, variation);
        positions.push_back(pos);
    }

    positions.push_back(end); // Always include end position

    // Sort positions to ensure they're in order
    std::sort(positions.begin(), positions.end());
    return positions;
}

int main(int argc, char* argv[]) {
    if (tb_init() != 0) {
        std::cerr << "Failed to initialize Termbox!" << std::endl;
        return 1;
    }

    // Default values
    std::vector<Color> selected_palette = OPAQUE_PALETTE;
    int density = 8;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--transparent" || arg == "-t") {
            selected_palette = TRANSPARENT_PALETTE;
        }
        else if (arg == "--density" || arg == "-d") {
            if (i + 1 < argc) {
                try {
                    density = std::stoi(argv[++i]);
                    if (density < 2) density = 2;
                    if (density > 50) density = 50;
                } catch (const std::exception& e) {
                    std::cerr << "Invalid density value. Must be between 2 and 50.\n";
                    tb_shutdown();
                    return 1;
                }
            } else {
                std::cerr << "Density value missing.\n";
                tb_shutdown();
                return 1;
            }
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                     << "Options:\n"
                     << "  --transparent, -t       Use transparent palette\n"
                     << "  --density N, -d N       Set density (2-50, default: 8)\n"
                     << "                          Lower numbers create busier art\n"
                     << "  --help, -h              Show this help message\n"
                     << "  (no arguments)          Use solid blocks (opaque style)\n";
            tb_shutdown();
            return 0;
        }
    }

    tb_clear();

    auto width = tb_width() - 1;
    auto height = tb_height() - 1;

    int vert_lines, horiz_lines;
    calculate_line_counts(width, height, vert_lines, horiz_lines, density);

    // Generate positions for vertical and horizontal divisions (now including edges)
    auto vertical_positions = generate_positions(0, width, vert_lines);
    auto horizontal_positions = generate_positions(0, height, horiz_lines);

    // Find rectangles directly from positions
    std::vector<Rectangle> rectangles;
    for (size_t i = 0; i < vertical_positions.size() - 1; i++) {
        for (size_t j = 0; j < horizontal_positions.size() - 1; j++) {
            rectangles.push_back({
                vertical_positions[i],
                horizontal_positions[j],
                vertical_positions[i + 1],
                horizontal_positions[j + 1],
                TRANSPARENT
            });
        }
    }

    // Merge and color rectangles
    merge_rectangles(rectangles);
    color_rectangles(rectangles, selected_palette);

    // Draw all rectangles
    for (const auto& rect : rectangles) {
        draw_rectangle(rect);
    }

    // Make the right edge a tad thicker
    for (int y = 0; y <= height; y++) {
        tb_change_cell(width - 1, y, ' ', TB_DEFAULT, TB_DEFAULT);
    }

    tb_present();
    struct tb_event event;
    tb_poll_event(&event);
    tb_shutdown();
    return 0;
}
