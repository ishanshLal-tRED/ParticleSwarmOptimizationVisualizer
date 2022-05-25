#include <logging.hxx>

import <filesystem>;
import <fstream>;
import <unordered_map>;
import <tuple>;
import <optional>;
import <string>;
#include <nlohmann/json.hpp>; // adl initializers conflict due to C++ modukes

import <glm/glm.hpp>; 

import Helpers.Files; 
export module Helpers.FontAtlasLoader;

namespace Helper { class FontAtlas; }

export typedef uint32_t unicode_t;
using json = nlohmann::json;
void to_json(json& j, const Helper::FontAtlas& p);
void from_json(const json& j, Helper::FontAtlas& p);

namespace Helper {
	export class FontAtlas {
	public:
		FontAtlas () = default;
		FontAtlas (const char* atlas_image, const char* json_config) { 
			if (!LoadFont (atlas_image, json_config)) 
				THROW_Critical ("cannot load atlas: don't use this constructor if you are not sure of atlas related files"); 
		}
		~FontAtlas () { Free (); }
		FontAtlas (const FontAtlas&) = delete;
		FontAtlas& operator= (const FontAtlas&) = delete;
		FontAtlas (FontAtlas&&) = default;
		FontAtlas& operator= (FontAtlas&&) = default;
		
		struct GlyphGeometry {
			struct { /// Bounds of the bounding box of the glyph as it should be placed on the baseline
				double left = 0, right = 0, top = 0, bottom = 0;
			} planeBounds;
			struct { /// Bounds of the bounding box of the glyph as it should be placed on the baseline
				double left = 0, right = 0, top = 0, bottom = 0;
			} atlasBounds;
			/// The glyph's advance
			float advance = 0;
			/// The Unicode codepoint represented by the glyph or 0 if unknown
			unicode_t unicode = 0;
		};

		bool LoadFont (const char* atlas_image, const char* json_config);
		void Free ();
		
		// Atlas related
		const std::string	AtlasType		() const { return atlas.type; }
		const int			AtlasPxRange	() const { return atlas.distanceRange; }
		const int			AtlasFontsize	() const { return atlas.size; }
		const int			AtlasWidth		() const { return atlas.width; }
		const int			AtlasHeight		() const { return atlas.height; }
		const std::string	AtlasYBaseline	() const { return atlas.yOrigin; }
		// Metrics related
		const int		MetricsEMsize		() const { return metrics.emSize; }
		const float		MetricsLineHeight	() const { return metrics.lineHeight; }
		const double	MetricsAscenderY	() const { return metrics.ascender; }
		const double	MetricsDescenderY	() const { return metrics.descender; }
		const double	MetricsUnderlineY	() const { return metrics.underlineY; }
		const double	MetricsUnderlineThickness () const { return metrics.underlineThickness; }

		// pixel_data, channel size
		const std::pair<void*, uint32_t> PixelData () const { return {pixels.data, pixels.channels}; }

		const GlyphGeometry* getGlyph (unicode_t code_point) const {
			if (glyphs.find (code_point) != glyphs.end ())
				return &(glyphs.at (code_point));
			return nullptr;
		}

	private:
		struct {
/* -type can be one of:
    hardmask – a non-anti-aliased binary image
    softmask – an anti-aliased image
    sdf – a true signed distance field (SDF)
    psdf – a pseudo-distance field
    msdf (default) – a multi-channel signed distance field (MSDF)
    mtsdf – a combination of MSDF and true SDF in the alpha channel */
			std::string type;

			/// pxRange - sets the distance field range in output pixels
			int distanceRange;

			/// fontsize <EM size> – sets the size of the glyphs in the atlas in pixels per EM
			int size;

			/// dimensions of image
			int width, height;

			/// yOrigin ? {YDirection::TOP_DOWN : "top", YDirection::BOTTOM_UP : "bottom"}
			std::string yOrigin;
		} atlas;

		std::string name = "<fontname>";

		struct { /// Loads font metrics and geometry scale from font
			/// The size of one EM.
			int emSize;
			/// The vertical difference between consecutive baselines.
	        double lineHeight;
			/// ascenderY, descenderY - The vertical position of the ascender and descender relative to the baseline.
	        double ascender, descender;
			/// The vertical position and thickness of the underline.
	        double underlineY, underlineThickness;
		} metrics;

		std::unordered_map <unicode_t, GlyphGeometry> glyphs;

		struct {
			/// Raw pixel data
			void* data = nullptr;

			/// No. of channels in image
			uint32_t channels;
		} pixels;
		
	private:
		friend void ::to_json (json& j, const Helper::FontAtlas& p);
		friend void ::from_json (const json& j, Helper::FontAtlas& p);
	};
};

bool Helper::FontAtlas::LoadFont (const char* atlas_image, const char* json_config) {
	Free ();

	if (!std::filesystem::exists (atlas_image)) {
		LOG_error ("cannot find Atlas Image at location: \'{}\'", atlas_image);
		return false;
	}
	if (!std::filesystem::exists (json_config)) {
		LOG_error ("cannot find Json Config at location: \'{}\'", json_config);
		return false;
	}

	json j; {
		std::ifstream i(json_config); i >> j;
	}
	
	{
		auto [data, w, h, c] = Helper::readImageRGBA (atlas_image);
		int wd, ht;
		j["atlas"]["width"].get_to (wd);
		j["atlas"]["height"].get_to (ht);
		if (wd != w || ht != h) {
			LOG_error ("target atlas image is conflicting in dimensions with width, height mentioned in json config.");
			LOG_raw ("\n\t Atlas Image: \'{}\', JSON Config: \'{}\'", atlas_image, json_config);
			LOG_raw ("\n\t Dimensions[Image: \'{:d}, {:d}\', Config: \'{:d}, {:d}\'", wd, ht, w, h);
			Helper::freeImage (data);
			return false;
		}
		pixels.data = data, pixels.channels = c;
	}

	//*this = j.get<decltype(*this)>();
	from_json (j, *this);
	return true;
}

void Helper::FontAtlas::Free () {
	if (pixels.data != nullptr) {
		Helper::freeImage ((uint8_t*)pixels.data);
		glyphs.clear ();
		FontAtlas empty;
		*this = std::move (empty);
	}
}

void to_json(json& j, const Helper::FontAtlas& fa) {
	j = json {
		{ "name", fa.name },
		{
			"atlas" , {
				{ "type", fa.atlas.type },
				{ "distanceRange", fa.atlas.distanceRange },
				{ "size", fa.atlas.size },
				{ "width", fa.atlas.width },
				{ "height", fa.atlas.height },
				{ "yOrigin", fa.atlas.yOrigin },
			},
		},
		{
			"metrics", {
				{ "emSize", fa.metrics.emSize },
				{ "lineHeight", fa.metrics.lineHeight },
				{ "ascender", fa.metrics.ascender },
				{ "descender", fa.metrics.descender },
				{ "underlineY", fa.metrics.underlineY },
				{ "underlineThickness", fa.metrics.underlineThickness },
			},
		},
	};
	
	json glyphs = j["glyphs"];
	glyphs.clear ();
	for (auto &pair: fa.glyphs) {
		const Helper::FontAtlas::GlyphGeometry &geom = pair.second;
		glyphs.push_back ( json {
			{ "unicode", geom.unicode },
			{ "advance", geom.advance },
			{ "planeBounds", {
					{ "left", geom.planeBounds.left },
					{ "bottom", geom.planeBounds.bottom },
					{ "right", geom.planeBounds.right },
					{ "top", geom.planeBounds.top },
				},
			},
			{ "atlasBounds", {
					{ "left", geom.atlasBounds.left },
					{ "bottom", geom.atlasBounds.bottom },
					{ "right", geom.atlasBounds.right },
					{ "top", geom.atlasBounds.top },
				},
			},
		});
	}
}

void from_json(const json& j, Helper::FontAtlas& fa) {
	j.at ("name").get_to(fa.name);
	{
		const json &atlas = j.at ("atlas");
		atlas.at ("type").get_to (fa.atlas.type);
		atlas.at ("distanceRange").get_to (fa.atlas.distanceRange);
		atlas.at ("size").get_to (fa.atlas.size);
		atlas.at ("width").get_to (fa.atlas.width);
		atlas.at ("height").get_to (fa.atlas.height);
		atlas.at ("yOrigin").get_to (fa.atlas.yOrigin);
	}
	{
		const json &metrics = j.at ("metrics");
		metrics.at ("emSize").get_to (fa.metrics.emSize);
		metrics.at ("lineHeight").get_to (fa.metrics.lineHeight);
		metrics.at ("ascender").get_to (fa.metrics.ascender);
		metrics.at ("descender").get_to (fa.metrics.descender);
		metrics.at ("underlineY").get_to (fa.metrics.underlineY);
		metrics.at ("underlineThickness").get_to (fa.metrics.underlineThickness);
	}
	{
		json glyphs = j.at ("glyphs");
		for (const json &glyph: glyphs) {
			Helper::FontAtlas::GlyphGeometry geom;
			glyph.at ("unicode").get_to (geom.unicode);
			glyph.at ("advance").get_to (geom.advance);
			
			if (auto planeBounds = glyph.find ("planeBounds"); planeBounds != glyph.end ()) {
				(*planeBounds).at ("left").get_to (geom.planeBounds.left);
				(*planeBounds).at ("bottom").get_to (geom.planeBounds.bottom);
				(*planeBounds).at ("right").get_to (geom.planeBounds.right);
				(*planeBounds).at ("top").get_to (geom.planeBounds.top);
			}
			if (auto atlasBounds = glyph.find ("atlasBounds"); atlasBounds != glyph.end ()) {
				(*atlasBounds).at ("left").get_to (geom.atlasBounds.left);
				(*atlasBounds).at ("bottom").get_to (geom.atlasBounds.bottom);
				(*atlasBounds).at ("right").get_to (geom.atlasBounds.right);
				(*atlasBounds).at ("top").get_to (geom.atlasBounds.top);
			}
		
			fa.glyphs[geom.unicode] = geom;
		}
	}
}