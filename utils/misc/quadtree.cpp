#include "quadtree.h"
#include "utils/common/bit_cast.h"
#include <array>
#include <cassert>

struct ImageView
{
	ImageView() = default;
	ImageView(const float *_data, size_t _width, size_t _height)
			: x_offset(0),
				y_offset(0),
				width(_width),
				height(_height),
				orig_width(_width),
				orig_height(_height),
				data(_data)
	{
	}
	ImageView(const ImageView &img, size_t _x_offset, size_t _y_offset, size_t _width, size_t _height)
			: x_offset(img.x_offset + _x_offset),
				y_offset(img.y_offset + _y_offset),
				width(_width),
				height(_height),
				orig_width(img.orig_width),
				orig_height(img.orig_height),
				data(img.data)
	{
	}
	float get(size_t x, size_t y)
	{
		x += x_offset;
		y += y_offset;
		float value = data[x + orig_width * y];
		if (value == 0.0f)
		{
		}
		return value;
	}
	size_t x_offset;
	size_t y_offset;
	size_t width;
	size_t height;
	size_t orig_width;
	size_t orig_height;
	const float *data;
};

std::array<ImageView, 4> split_image(ImageView img)
{
	size_t x_split = img.width / 2;
	size_t y_split = img.height / 2;
	ImageView bottom_left(img, 0, 0, x_split, y_split);
	ImageView bottom_right(img, x_split, 0, img.width - x_split, y_split);
	ImageView top_left(img, 0, y_split, x_split, img.height - y_split);
	ImageView top_right(img, x_split, y_split, img.width - x_split, img.height - y_split);
	return {
			bottom_left,
			bottom_right,
			top_right,
			top_left};
}

uint32_t add_value(std::vector<uint32_t> &data, uint32_t value)
{
	size_t offset = data.size();
	data.push_back(value);
	return offset;
}

uint32_t create_quadtree_from_image(std::vector<uint32_t> &nodes, ImageView img)
{
	if (img.width == 0 || img.height == 0)
	{
		assert(false);
		return (uint32_t)-1;
	}
	if (img.width == 1 || img.height == 1)
	{
		return add_value(nodes, bit_cast<uint32_t>(img.get(0, 0)));
	}
	else if (img.width == 2 || img.height == 2)
	{
		uint32_t first = add_value(nodes, bit_cast<uint32_t>(img.get(0, 0)));
		add_value(nodes, bit_cast<uint32_t>(img.get(1, 0)));
		add_value(nodes, bit_cast<uint32_t>(img.get(1, 1)));
		add_value(nodes, bit_cast<uint32_t>(img.get(0, 1)));
		return first;
	}
	else
	{
		auto [bottom_left, bottom_right, top_right, top_left] = split_image(img);
		uint32_t bl_root = create_quadtree_from_image(nodes, bottom_left);
		uint32_t br_root = create_quadtree_from_image(nodes, bottom_right);
		uint32_t tr_root = create_quadtree_from_image(nodes, top_right);
		uint32_t tl_root = create_quadtree_from_image(nodes, top_left);
		size_t first_offset = add_value(nodes, bl_root | QUADTREE_LEAF_BIT);
		add_value(nodes, br_root);
		add_value(nodes, tr_root);
		add_value(nodes, tl_root);
		return first_offset;
	}
}

std::vector<uint32_t> create_quadtree_from_image(const float *image, size_t w, size_t h)
{
	std::vector<uint32_t> out(1);
	uint32_t root_index = create_quadtree_from_image(out, ImageView(image, w, h));
	out[0] = root_index;
	return out;
}