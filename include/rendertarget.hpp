#pragma once

#include "raylib.h"

// RAII wrapper around RenderTexture2D so unloading can't be forgotten or done twice
// across the resize/recreate cycle in syncScreenSize.
class RenderTarget
{
  public:
    RenderTarget() = default;
    RenderTarget(const RenderTarget&) = delete;
    auto operator=(const RenderTarget&) -> RenderTarget& = delete;
    RenderTarget(RenderTarget&& other) noexcept : texture_(other.texture_) { other.texture_ = {}; }
    auto operator=(RenderTarget&& other) noexcept -> RenderTarget&
    {
        if (this != &other)
        {
            reset();
            texture_ = other.texture_;
            other.texture_ = {};
        }
        return *this;
    }
    ~RenderTarget() { reset(); }

    void load(int width, int height)
    {
        reset();
        texture_ = LoadRenderTexture(width, height);
    }

    void reset()
    {
        if (texture_.id != 0)
        {
            UnloadRenderTexture(texture_);
            texture_ = RenderTexture2D{};
        }
    }

    [[nodiscard]] auto get() const -> const RenderTexture2D& { return texture_; }

    operator RenderTexture2D() const { return texture_; }

  private:
    RenderTexture2D texture_{};
};
