#include "browser/graphics/rasterizer.hpp"
#include "browser/core/diagnostics.hpp"
#include "browser/dom/element.hpp"

#include <SDL3/SDL.h>
#include <stack>
#include <functional>
#include <algorithm>
#include <cmath>

namespace browser::graphics {

namespace {

void renderRoundedFRect(SDL_Renderer* renderer, float x, float y, float w, float h, float radius, const core::Color& color, bool filled) {
    radius = std::clamp(radius, 0.0f, std::min(w, h) / 2.0f);
    if (radius <= 0.0f) {
        SDL_FRect frect{x, y, w, h};
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        if (filled) SDL_RenderFillRect(renderer, &frect);
        else SDL_RenderRect(renderer, &frect);
        return;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    float r = radius;

    if (filled) {
        SDL_FRect center{x + r, y, w - 2.0f * r, h};
        SDL_FRect left{x, y + r, r, h - 2.0f * r};
        SDL_FRect right{x + w - r, y + r, r, h - 2.0f * r};
        SDL_RenderFillRect(renderer, &center);
        SDL_RenderFillRect(renderer, &left);
        SDL_RenderFillRect(renderer, &right);

        int ir = static_cast<int>(r);
        for (int dy = 0; dy <= ir; ++dy) {
            float dx = std::sqrt(std::max(0.0f, r * r - static_cast<float>(dy * dy)));
            SDL_RenderLine(renderer, x + r - dx, y + r - static_cast<float>(dy), x + r, y + r - static_cast<float>(dy));
            SDL_RenderLine(renderer, x + w - r, y + r - static_cast<float>(dy), x + w - r + dx, y + r - static_cast<float>(dy));
            SDL_RenderLine(renderer, x + r - dx, y + h - r + static_cast<float>(dy), x + r, y + h - r + static_cast<float>(dy));
            SDL_RenderLine(renderer, x + w - r, y + h - r + static_cast<float>(dy), x + w - r + dx, y + h - r + static_cast<float>(dy));
        }
    } else {
        SDL_RenderLine(renderer, x + r, y, x + w - r, y);
        SDL_RenderLine(renderer, x + r, y + h, x + w - r, y + h);
        SDL_RenderLine(renderer, x, y + r, x, y + h - r);
        SDL_RenderLine(renderer, x + w, y + r, x + w, y + h - r);

        int steps = std::max(6, static_cast<int>(r * 1.5f));
        float stepsF = static_cast<float>(steps);
        for (int i = 0; i < steps; ++i) {
            float a1 = (static_cast<float>(i) / stepsF) * (3.14159265f / 2.0f);
            float a2 = (static_cast<float>(i + 1) / stepsF) * (3.14159265f / 2.0f);
            float cos1 = std::cos(a1) * r, sin1 = std::sin(a1) * r;
            float cos2 = std::cos(a2) * r, sin2 = std::sin(a2) * r;

            SDL_RenderLine(renderer, x + r - cos1, y + r - sin1, x + r - cos2, y + r - sin2);
            SDL_RenderLine(renderer, x + w - r + cos1, y + r - sin1, x + w - r + cos2, y + r - sin2);
            SDL_RenderLine(renderer, x + r - cos1, y + h - r + sin1, x + r - cos2, y + h - r + sin2);
            SDL_RenderLine(renderer, x + w - r + cos1, y + h - r + sin1, x + w - r + cos2, y + h - r + sin2);
        }
    }
}

} // namespace

void Rasterizer::rasterize(
    SDL_Renderer* renderer,
    const DisplayList& displayList,
    const core::Rect& viewport,
    const core::Point& scrollOffset
) {
    if (!renderer) return;
    core::ScopedTimer timer(core::Subsystem::Painting);

    // Set viewport clip rect
    SDL_Rect sdlViewport{viewport.x, viewport.y, viewport.width, viewport.height};
    SDL_SetRenderClipRect(renderer, &sdlViewport);

    std::stack<SDL_Rect> clipStack;
    clipStack.push(sdlViewport);

    for (const auto& item : displayList.items()) {
        switch (item.type) {

            case ItemType::FillRect: {
                const auto* d = item.get<FillRectItem>();
                float rx = static_cast<float>(viewport.x) + d->rect.x - static_cast<float>(scrollOffset.x);
                float ry = static_cast<float>(viewport.y) + d->rect.y - static_cast<float>(scrollOffset.y);

                if (d->borderRadius > 0.0f) {
                    renderRoundedFRect(renderer, rx, ry, d->rect.width, d->rect.height, d->borderRadius, d->color, true);
                } else {
                    SDL_FRect frect{rx, ry, d->rect.width, d->rect.height};
                    SDL_SetRenderDrawColor(renderer, d->color.r, d->color.g, d->color.b, d->color.a);
                    SDL_RenderFillRect(renderer, &frect);
                }
                break;
            }

            case ItemType::StrokeRect: {
                const auto* d = item.get<StrokeRectItem>();
                float rx = static_cast<float>(viewport.x) + d->rect.x - static_cast<float>(scrollOffset.x);
                float ry = static_cast<float>(viewport.y) + d->rect.y - static_cast<float>(scrollOffset.y);

                if (d->borderRadius > 0.0f) {
                    renderRoundedFRect(renderer, rx, ry, d->rect.width, d->rect.height, d->borderRadius, d->color, false);
                } else {
                    SDL_FRect frect{rx, ry, d->rect.width, d->rect.height};
                    SDL_SetRenderDrawColor(renderer, d->color.r, d->color.g, d->color.b, d->color.a);
                    SDL_RenderRect(renderer, &frect);
                }
                break;
            }

            case ItemType::Text: {
                const auto* d = item.get<TextItem>();
                float tx = static_cast<float>(viewport.x) + d->bounds.x - static_cast<float>(scrollOffset.x);
                float ty = static_cast<float>(viewport.y) + d->bounds.y - static_cast<float>(scrollOffset.y);

                SDL_SetRenderDrawColor(renderer, d->color.r, d->color.g, d->color.b, d->color.a);

                // Scale text rendering according to font size (SDL_RenderDebugText default glyph size is 8x8)
                float scale = std::max(1.0f, d->fontSize / 16.0f);
                float oldScaleX = 1.0f, oldScaleY = 1.0f;
                SDL_GetRenderScale(renderer, &oldScaleX, &oldScaleY);

                SDL_SetRenderScale(renderer, oldScaleX * scale, oldScaleY * scale);
                SDL_RenderDebugText(renderer, tx / scale, ty / scale, d->text.c_str());
                SDL_SetRenderScale(renderer, oldScaleX, oldScaleY);
                break;
            }

            case ItemType::PushClip: {
                const auto* d = item.get<PushClipItem>();
                int cx = viewport.x + static_cast<int>(d->clipRect.x) - scrollOffset.x;
                int cy = viewport.y + static_cast<int>(d->clipRect.y) - scrollOffset.y;
                int cw = static_cast<int>(d->clipRect.width);
                int ch = static_cast<int>(d->clipRect.height);

                SDL_Rect newClip{cx, cy, cw, ch};
                clipStack.push(newClip);
                SDL_SetRenderClipRect(renderer, &newClip);
                break;
            }

            case ItemType::PopClip: {
                if (clipStack.size() > 1) {
                    clipStack.pop();
                    SDL_SetRenderClipRect(renderer, &clipStack.top());
                }
                break;
            }
        }
    }

    // Reset clip rect to entire window
    SDL_SetRenderClipRect(renderer, nullptr);
}

const dom::Element* Rasterizer::hitTest(
    const layout::LayoutBox& root,
    const core::Point& pos,
    const core::Rect& viewport,
    const core::Point& scrollOffset
) {
    if (!viewport.contains(pos)) {
        return nullptr;
    }

    float pageX = static_cast<float>(pos.x - viewport.x + scrollOffset.x);
    float pageY = static_cast<float>(pos.y - viewport.y + scrollOffset.y);

    std::function<const dom::Element*(const layout::LayoutBox&)> testBox =
        [&](const layout::LayoutBox& box) -> const dom::Element* {
            // Check reverse order of children (front to back)
            for (auto it = box.children().rbegin(); it != box.children().rend(); ++it) {
                if (auto hit = testBox(**it)) {
                    return hit;
                }
            }

            if (box.dimensions().borderBox().contains(pageX, pageY)) {
                if (box.node() && box.node()->nodeType() == dom::NodeType::Element) {
                    return static_cast<const dom::Element*>(box.node().get());
                }
            }
            return nullptr;
        };

    return testBox(root);
}

} // namespace browser::graphics
