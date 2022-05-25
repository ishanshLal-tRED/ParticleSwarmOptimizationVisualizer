#include <logging.hxx>

import <filesystem>;
import <fstream>;
export import <vector>;
export import <tuple>;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h";

export module Helpers.Files;

namespace Helper {
	export
	std::vector<char> readFile(const std::string &filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			THROW_Critical ("unable to open file!");

		size_t file_size = size_t(file.tellg());
		std::vector<char> buffer(file_size);
		file.seekg(0);
		
		file.read (buffer.data (), file_size);
		
		file.close ();
		return buffer;
	}

	export /// returns {image_data, width, height, channels}
	std::tuple<uint8_t*, uint32_t, uint32_t, uint32_t> readImageRGBA (const std::string &filename) {
		int width, height, channels;
		//stbi_set_flip_vertically_on_load(true);
		stbi_uc* image_data = stbi_load (filename.c_str (), &width, &height, &channels, STBI_rgb_alpha);
		if (image_data == nullptr)
			THROW_Critical ("couldn't load image: \'{:s}\'", filename);

		return {image_data, width, height, channels};
	}

	export
	void freeImage (uint8_t* image_data) {
		stbi_image_free (image_data);
	}
}