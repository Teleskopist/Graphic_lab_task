#pragma once
#include "lite_app.h"
#include "imgui/imgui.h"
#include "blk/blk.h"

namespace LiteApp
{
	void init_ImGui(RenderingContext &r_ctx);
	VkCommandBuffer build_command_buffer_imGui(uint32_t a_swapchainFrameIdx, void* a_userData, const RenderingContext &r_ctx);
	void on_swapchain_changed_imGui(RenderingContext &r_ctx);
	void cleanup_imGui(RenderingContext &r_ctx);
	void blk_modification_interface(Block *b, const std::string &title);
}