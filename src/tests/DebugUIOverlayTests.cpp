#include <catch2/catch_test_macros.hpp>

#include "Debug/DebugUIOverlay.h"
#include "GraphicsTestEnv.h"

using namespace Muyo;

// Regression guard for muyo-renderer#8. The Debug app aborted at startup because IMGUI_USER_CONFIG
// was set on `imgui` and `tests` only: the config changes *inline* definitions in imgui.h, so the
// font atlas built in imgui's translation units held ImTextureID_Invalid = -1 while the overlay's
// translation unit, compiled without the config, asserted against the default sentinel 0.
// Constructing the overlay is the cross-translation-unit interaction that exposes the mismatch;
// no earlier case did, so the suite stayed green while the app aborted (the reviews built the app
// but never ran it). With the define missing from `muyo_renderer` this case aborts in Debug at the
// IM_ASSERT inside ImFontAtlas::SetTexID — the same line the app died on.

TEST_CASE_METHOD(GraphicsTestEnv, "DebugUIOverlay: the font atlas and the overlay agree on the invalid texture id",
                 "[DebugUI]")
{
    // The sentinel this project configures (src/ImGuiUserConfig.h): 0 is a valid texture id — the
    // UI font — so "invalid" moves to -1.
    CHECK(ImTextureID_Invalid == static_cast<ImTextureID>(-1));

    // The constructor builds the atlas (imgui's translation units), creates the font texture
    // through the resource manager, and calls SetTexID(0) from muyo_renderer's translation unit.
    DebugUIOverlay overlay;
    REQUIRE(overlay.GetFontTexture() != nullptr);
    CHECK(ImGui::GetIO().Fonts->TexRef.GetTexID() == static_cast<ImTextureID>(0));
}
