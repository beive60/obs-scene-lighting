#include <obs-module.h>
#include <obs-frontend-api.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <graphics/vec4.h>
#include <util/platform.h>

#include <cmath>
#include <cstdint>
#include <cstring>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-scene-lighting", "en-US")

/**
 * OBS property key for choosing a background source.
 */
static constexpr const char *kPropBackgroundSource = "background_source";

/**
 * OBS property key for blend mode.
 */
static constexpr const char *kPropBlendMode = "blend_mode";

/**
 * OBS property key for intensity percent.
 */
static constexpr const char *kPropIntensity = "intensity";

/**
 * OBS property key for blur radius.
 */
static constexpr const char *kPropBlurRadius = "blur_radius";

/**
 * OBS property key for edge width.
 */
static constexpr const char *kPropEdgeWidth = "edge_width";

/**
 * OBS property key for edge threshold.
 */
static constexpr const char *kPropEdgeThreshold = "edge_threshold";

/**
 * OBS property key for light wrap toggle.
 */
static constexpr const char *kPropEnableWrap = "enable_wrap";

/**
 * OBS property key for rim light toggle.
 */
static constexpr const char *kPropEnableRim = "enable_rim";

/**
 * OBS property key for rim light angle.
 */
static constexpr const char *kPropRimAngle = "rim_angle";

/**
 * OBS property key for tint color.
 */
static constexpr const char *kPropTintColor = "tint_color";

/**
 * OBS property key for sampling method.
 */
static constexpr const char *kPropSamplingMethod = "sampling_method";

/**
 * Filter ID used for OBS source registration.
 */
static constexpr const char *kFilterId = "scene_lighting_filter";

/**
 * Localization key for filter display name.
 */
static constexpr const char *kTextFilterName = "SceneLighting.FilterName";

/**
 * Localization key for module description.
 */
static constexpr const char *kTextDescription = "SceneLighting.Description";

/**
 * Localization key for background source label.
 */
static constexpr const char *kTextBackgroundSource = "SceneLighting.BackgroundSource";

/**
 * Localization key for blend mode label.
 */
static constexpr const char *kTextBlendMode = "SceneLighting.BlendMode";

/**
 * Localization key for intensity label.
 */
static constexpr const char *kTextIntensity = "SceneLighting.Intensity";

/**
 * Localization key for blur radius label.
 */
static constexpr const char *kTextBlurRadius = "SceneLighting.BlurRadius";

/**
 * Localization key for edge width label.
 */
static constexpr const char *kTextEdgeWidth = "SceneLighting.EdgeWidth";

/**
 * Localization key for edge threshold label.
 */
static constexpr const char *kTextEdgeThreshold = "SceneLighting.EdgeThreshold";

/**
 * Localization key for light wrap label.
 */
static constexpr const char *kTextLightWrap = "SceneLighting.LightWrap";

/**
 * Localization key for rim light label.
 */
static constexpr const char *kTextRimLight = "SceneLighting.RimLight";

/**
 * Localization key for rim angle label.
 */
static constexpr const char *kTextRimAngle = "SceneLighting.RimAngle";

/**
 * Localization key for tint color label.
 */
static constexpr const char *kTextTintColor = "SceneLighting.TintColor";

/**
 * Localization key for sampling method label.
 */
static constexpr const char *kTextSamplingMethod = "SceneLighting.SamplingMethod";

/**
 * Localization key for empty source option.
 */
static constexpr const char *kTextNone = "SceneLighting.None";

/**
 * Localization key for multiply blend option.
 */
static constexpr const char *kTextBlendMultiply = "SceneLighting.BlendMode.Multiply";

/**
 * Localization key for screen blend option.
 */
static constexpr const char *kTextBlendScreen = "SceneLighting.BlendMode.Screen";

/**
 * Localization key for overlay blend option.
 */
static constexpr const char *kTextBlendOverlay = "SceneLighting.BlendMode.Overlay";

/**
 * Localization key for add blend option.
 */
static constexpr const char *kTextBlendAdd = "SceneLighting.BlendMode.Add";

/**
 * Localization key for average sampling option.
 */
static constexpr const char *kTextSamplingAverage = "SceneLighting.Sampling.Average";

/**
 * Localization key for center-point sampling option.
 */
static constexpr const char *kTextSamplingCenter = "SceneLighting.Sampling.Center";

/**
 * Maximum supported edge width.
 */
static constexpr float kMaxEdgeWidth = 50.0F;

/**
 * Radians conversion factor for degrees.
 */
static constexpr float kPi = 3.14159265358979323846F;

/**
 * Clamps integer value into the provided range.
 */
static int32_t clamp_int(int32_t value, int32_t min_value, int32_t max_value)
{
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

/**
 * Blend mode options aligned with the AviUtl2 script.
 */
enum class BlendMode : int32_t {
	/**
	 * Multiply blending mode.
	 */
	Multiply = 0,
	/**
	 * Screen blending mode.
	 */
	Screen = 1,
	/**
	 * Overlay blending mode.
	 */
	Overlay = 2,
	/**
	 * Additive blending mode.
	 */
	Add = 3,
};

/**
 * Sampling mode options aligned with the AviUtl2 script.
 */
enum class SamplingMode : int32_t {
	/**
	 * Grid average sampling mode.
	 */
	Average = 0,
	/**
	 * Five-point center weighted sampling mode.
	 */
	CenterPoints = 1,
};

/**
 * Runtime state for the scene lighting filter.
 */
struct SceneLightingFilter {
	/**
	 * OBS source context for this filter.
	 */
	obs_source_t *source = nullptr;
	/**
	 * Name of the configured background source.
	 */
	char *background_source_name = nullptr;
	/**
	 * Weak reference to configured background source.
	 */
	obs_weak_source_t *background_source = nullptr;
	/**
	 * Guards against re-entrant background rendering (e.g. when the
	 * background source transitively contains this filter's parent).
	 */
	bool rendering_background = false;
	/**
	 * Loaded graphics effect.
	 */
	gs_effect_t *effect = nullptr;
	/**
	 * Offscreen render target for background source.
	 */
	gs_texrender_t *background_render_target = nullptr;
	/**
	 * Configured blend mode.
	 */
	BlendMode blend_mode = BlendMode::Screen;
	/**
	 * Effect intensity in percent.
	 */
	int32_t intensity_percent = 50;
	/**
	 * Sampling blur radius in pixels.
	 */
	int32_t blur_radius = 15;
	/**
	 * Edge width in pixels.
	 */
	int32_t edge_width = 10;
	/**
	 * Edge alpha threshold in byte units.
	 */
	int32_t edge_threshold = 16;
	/**
	 * Whether light wrap is enabled.
	 */
	bool enable_wrap = true;
	/**
	 * Whether rim lighting is enabled.
	 */
	bool enable_rim = true;
	/**
	 * Rim light direction angle in degrees.
	 */
	int32_t rim_angle = 45;
	/**
	 * RGB tint color packed into BGR integer.
	 */
	uint32_t tint_color = 0xFFFFFF;
	/**
	 * Sampling method for ambient extraction.
	 */
	SamplingMode sampling_method = SamplingMode::Average;
};

/**
 * Maps source UV coordinates into the root-scene UV space.
 */
struct SceneUvMapping {
	bool available = false;
	uint32_t background_width = 0;
	uint32_t background_height = 0;
	struct vec2 origin = {0.0F, 0.0F};
	struct vec2 axis_x = {1.0F, 0.0F};
	struct vec2 axis_y = {0.0F, 1.0F};
};

/**
 * Represents a background scene item's oriented box in scene pixels.
 */
struct BackgroundSceneBox {
	bool available = false;
	struct vec2 center = {0.0F, 0.0F};
	struct vec2 axis_x = {1.0F, 0.0F};
	struct vec2 axis_y = {0.0F, 1.0F};
	struct vec2 half_extent = {0.0F, 0.0F};
};

/**
 * Tracks recursive scene-item traversal while looking for a source.
 */
struct SceneItemSearchContext {
	obs_source_t *source = nullptr;
	size_t match_count = 0;
	struct matrix4 found_draw_transform = {};
	struct matrix4 found_box_transform = {};
};

/**
 * Carries the accumulated draw transform through scene traversal.
 */
struct SceneTraversalState {
	SceneItemSearchContext *context = nullptr;
	struct matrix4 accumulated_transform = {};
};

enum class SceneSearchStatus {
	NotFound,
	Unique,
	Ambiguous,
};

/**
 * Holds a unique scene-root match and its render dimensions.
 */
struct RootSceneItemMatch {
	struct matrix4 draw_transform = {};
	struct matrix4 box_transform = {};
	uint32_t width = 0;
	uint32_t height = 0;
};

/**
 * Returns plugin description shown in OBS UI.
 */
MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text(kTextDescription);
}

/**
 * Releases current weak reference and binds to configured source name.
 */
static void update_background_source(SceneLightingFilter *filter)
{
if (filter->background_source != nullptr) {
obs_weak_source_release(filter->background_source);
filter->background_source = nullptr;
}

if (filter->background_source_name == nullptr || filter->background_source_name[0] == '\0') {
return;
}

obs_source_t *source = obs_get_source_by_name(filter->background_source_name);
if (source == nullptr) {
return;
}

filter->background_source = obs_source_get_weak_source(source);
obs_source_release(source);
}

/**
 * Copies source name from settings into filter storage.
 */
static void set_background_source_name(SceneLightingFilter *filter, const char *name)
{
bfree(filter->background_source_name);
filter->background_source_name = nullptr;

if (name == nullptr || name[0] == '\0') {
return;
}

filter->background_source_name = bstrdup(name);
}

/**
 * Converts packed color integer to normalized vector components.
 */
static void decode_tint(uint32_t tint_color, float &r, float &g, float &b)
{
const float normalize = 1.0F / 255.0F;
r = static_cast<float>(tint_color & 0xFFU) * normalize;
g = static_cast<float>((tint_color >> 8U) & 0xFFU) * normalize;
b = static_cast<float>((tint_color >> 16U) & 0xFFU) * normalize;
}

/**
 * Converts a transformed point into normalized UV space for the background texture.
 */
static struct vec2 normalize_scene_point(const struct vec4 &point, uint32_t width, uint32_t height)
{
	const float reciprocal_w = std::abs(point.w) > 0.00001F ? 1.0F / point.w : 1.0F;
	return {point.x * reciprocal_w / static_cast<float>(width),
		point.y * reciprocal_w / static_cast<float>(height)};
}

/**
 * Converts a transformed point into scene pixel coordinates.
 */
static struct vec2 project_scene_point(const struct matrix4 &transform, float x, float y)
{
	struct vec4 point = {x, y, 0.0F, 1.0F};
	struct vec4 projected;
	vec4_transform(&projected, &point, &transform);
	const float reciprocal_w = std::abs(projected.w) > 0.00001F ? 1.0F / projected.w : 1.0F;
	return {projected.x * reciprocal_w, projected.y * reciprocal_w};
}

/**
 * Reads the root-scene render size, falling back to the OBS base canvas size.
 */
static bool get_root_scene_dimensions(obs_source_t *scene_source, uint32_t &width, uint32_t &height)
{
	width = 0;
	height = 0;
	if (scene_source != nullptr) {
		width = obs_source_get_base_width(scene_source);
		height = obs_source_get_base_height(scene_source);
	}

	if (width != 0 && height != 0) {
		return true;
	}

	obs_video_info video_info = {};
	if (!obs_get_video_info(&video_info)) {
		return false;
	}

	width = video_info.base_width;
	height = video_info.base_height;
	return width != 0 && height != 0;
}

/**
 * Recursively searches scene items, groups, and nested scenes for a source.
 */
static bool find_scene_item_recursive(obs_scene_t *scene, obs_sceneitem_t *item, void *param)
{
	UNUSED_PARAMETER(scene);
	auto *state = static_cast<SceneTraversalState *>(param);
	struct matrix4 item_transform;
	struct matrix4 item_box_transform;
	obs_sceneitem_get_draw_transform(item, &item_transform);
	obs_sceneitem_get_box_transform(item, &item_box_transform);

	struct matrix4 combined_transform;
	struct matrix4 combined_box_transform;
	matrix4_mul(&combined_transform, &state->accumulated_transform, &item_transform);
	matrix4_mul(&combined_box_transform, &state->accumulated_transform, &item_box_transform);

	obs_source_t *item_source = obs_sceneitem_get_source(item);
	if (item_source == state->context->source) {
		if (state->context->match_count == 0) {
			matrix4_copy(&state->context->found_draw_transform, &combined_transform);
			matrix4_copy(&state->context->found_box_transform, &combined_box_transform);
		}
		state->context->match_count++;
		if (state->context->match_count > 1) {
			return false;
		}
	}

	SceneTraversalState child_state = *state;
	matrix4_copy(&child_state.accumulated_transform, &combined_transform);

	if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, find_scene_item_recursive, &child_state);
		return state->context->match_count <= 1;
	}

	if (item_source != nullptr && obs_source_is_scene(item_source)) {
		obs_scene_t *nested_scene = obs_scene_from_source(item_source);
		if (nested_scene != nullptr) {
			obs_scene_enum_items(nested_scene, find_scene_item_recursive, &child_state);
		}
	}

	return state->context->match_count <= 1;
}

/**
 * Looks for a unique instance of the filtered source inside a root scene.
 */
static SceneSearchStatus search_root_scene_item(obs_source_t *root_scene_source,
	obs_source_t *target_source, RootSceneItemMatch &match)
{
	if (root_scene_source == nullptr || target_source == nullptr) {
		return SceneSearchStatus::NotFound;
	}

	obs_scene_t *root_scene = obs_scene_from_source(root_scene_source);
	if (root_scene == nullptr) {
		return SceneSearchStatus::NotFound;
	}

	SceneItemSearchContext context = {};
	context.source = target_source;
	matrix4_identity(&context.found_draw_transform);
	matrix4_identity(&context.found_box_transform);

	SceneTraversalState state = {};
	state.context = &context;
	matrix4_identity(&state.accumulated_transform);
	obs_scene_enum_items(root_scene, find_scene_item_recursive, &state);

	if (context.match_count == 0) {
		return SceneSearchStatus::NotFound;
	}
	if (context.match_count > 1) {
		return SceneSearchStatus::Ambiguous;
	}
	if (!get_root_scene_dimensions(root_scene_source, match.width, match.height)) {
		return SceneSearchStatus::NotFound;
	}

	matrix4_copy(&match.draw_transform, &context.found_draw_transform);
	matrix4_copy(&match.box_transform, &context.found_box_transform);
	return SceneSearchStatus::Unique;
}

/**
 * Converts a unique root-scene transform into background UV mapping vectors.
 */
static void build_scene_uv_mapping(const RootSceneItemMatch &match, uint32_t local_width,
	uint32_t local_height, SceneUvMapping &mapping)
{
	struct vec4 local_origin = {0.0F, 0.0F, 0.0F, 1.0F};
	struct vec4 local_x = {static_cast<float>(local_width), 0.0F, 0.0F, 1.0F};
	struct vec4 local_y = {0.0F, static_cast<float>(local_height), 0.0F, 1.0F};
	struct vec4 scene_origin;
	struct vec4 scene_x;
	struct vec4 scene_y;
	vec4_transform(&scene_origin, &local_origin, &match.draw_transform);
	vec4_transform(&scene_x, &local_x, &match.draw_transform);
	vec4_transform(&scene_y, &local_y, &match.draw_transform);

	mapping.available = true;
	mapping.background_width = match.width;
	mapping.background_height = match.height;
	mapping.origin = normalize_scene_point(scene_origin, match.width, match.height);
	const struct vec2 scene_x_uv = normalize_scene_point(scene_x, match.width, match.height);
	const struct vec2 scene_y_uv = normalize_scene_point(scene_y, match.width, match.height);
	mapping.axis_x = {scene_x_uv.x - mapping.origin.x, scene_x_uv.y - mapping.origin.y};
	mapping.axis_y = {scene_y_uv.x - mapping.origin.x, scene_y_uv.y - mapping.origin.y};
}

/**
 * Builds an oriented scene-space box from a unique background item match.
 */
static bool build_background_scene_box(const RootSceneItemMatch &match, BackgroundSceneBox &box)
{
	const struct vec2 corner00 = project_scene_point(match.box_transform, 0.0F, 0.0F);
	const struct vec2 corner10 = project_scene_point(match.box_transform, 1.0F, 0.0F);
	const struct vec2 corner01 = project_scene_point(match.box_transform, 0.0F, 1.0F);
	const struct vec2 axis_x = {corner10.x - corner00.x, corner10.y - corner00.y};
	const struct vec2 axis_y = {corner01.x - corner00.x, corner01.y - corner00.y};
	const float axis_x_length = std::sqrt(axis_x.x * axis_x.x + axis_x.y * axis_x.y);
	const float axis_y_length = std::sqrt(axis_y.x * axis_y.x + axis_y.y * axis_y.y);
	if (axis_x_length <= 0.00001F || axis_y_length <= 0.00001F) {
		return false;
	}

	box.available = true;
	box.axis_x = {axis_x.x / axis_x_length, axis_x.y / axis_x_length};
	box.axis_y = {axis_y.x / axis_y_length, axis_y.y / axis_y_length};
	box.half_extent = {axis_x_length * 0.5F, axis_y_length * 0.5F};
	box.center = {corner00.x + axis_x.x * 0.5F + axis_y.x * 0.5F,
		corner00.y + axis_x.y * 0.5F + axis_y.y * 0.5F};
	return true;
}

/**
 * Resolves a source to a unique scene item and optionally returns the root scene.
 */
static bool try_resolve_scene_item_match(obs_source_t *source, RootSceneItemMatch &match,
	obs_source_t **resolved_root_scene)
{
	if (resolved_root_scene != nullptr) {
		*resolved_root_scene = nullptr;
	}
	if (source == nullptr) {
		return false;
	}

	auto release_scene = [](obs_source_t *scene_source) {
		if (scene_source != nullptr) {
			obs_source_release(scene_source);
		}
	};

	obs_source_t *current_scene = obs_frontend_get_current_scene();
	const SceneSearchStatus current_status = search_root_scene_item(current_scene, source, match);
	if (current_status == SceneSearchStatus::Unique) {
		if (resolved_root_scene != nullptr) {
			*resolved_root_scene = current_scene;
		} else {
			release_scene(current_scene);
		}
		return true;
	}
	if (current_status == SceneSearchStatus::Ambiguous) {
		release_scene(current_scene);
		return false;
	}

	obs_source_t *preview_scene = nullptr;
	if (obs_frontend_preview_program_mode_active()) {
		preview_scene = obs_frontend_get_current_preview_scene();
		if (preview_scene != current_scene) {
			const SceneSearchStatus preview_status = search_root_scene_item(preview_scene, source, match);
			if (preview_status == SceneSearchStatus::Unique) {
				release_scene(current_scene);
				if (resolved_root_scene != nullptr) {
					*resolved_root_scene = preview_scene;
				} else {
					release_scene(preview_scene);
				}
				return true;
			}
			if (preview_status == SceneSearchStatus::Ambiguous) {
				release_scene(preview_scene);
				release_scene(current_scene);
				return false;
			}
		} else {
			release_scene(preview_scene);
			preview_scene = nullptr;
		}
	}

	bool found_match = false;
	obs_source_t *matched_scene = nullptr;
	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	for (size_t index = 0; index < scenes.sources.num; ++index) {
		obs_source_t *scene_source = scenes.sources.array[index];
		if (scene_source == current_scene || scene_source == preview_scene) {
			continue;
		}

		RootSceneItemMatch candidate = {};
		const SceneSearchStatus status = search_root_scene_item(scene_source, source, candidate);
		if (status == SceneSearchStatus::Ambiguous) {
			obs_frontend_source_list_free(&scenes);
			release_scene(matched_scene);
			release_scene(preview_scene);
			release_scene(current_scene);
			return false;
		}
		if (status == SceneSearchStatus::Unique) {
			if (found_match) {
				obs_frontend_source_list_free(&scenes);
				release_scene(matched_scene);
				release_scene(preview_scene);
				release_scene(current_scene);
				return false;
			}
			found_match = true;
			match = candidate;
			matched_scene = obs_source_get_ref(scene_source);
		}
	}
	obs_frontend_source_list_free(&scenes);

	release_scene(preview_scene);
	release_scene(current_scene);

	if (!found_match) {
		release_scene(matched_scene);
		return false;
	}

	if (resolved_root_scene != nullptr) {
		*resolved_root_scene = matched_scene;
	} else {
		release_scene(matched_scene);
	}
	return true;
}

/**
 * Resolves the filtered source into the active scene graph and builds UV mapping.
 */
static bool try_resolve_scene_uv_mapping(obs_source_t *source, uint32_t local_width, uint32_t local_height,
	SceneUvMapping &mapping, obs_source_t **resolved_root_scene = nullptr)
{
	if (source == nullptr || local_width == 0 || local_height == 0) {
		return false;
	}

	RootSceneItemMatch match = {};
	if (!try_resolve_scene_item_match(source, match, resolved_root_scene)) {
		return false;
	}

	build_scene_uv_mapping(match, local_width, local_height, mapping);
	return true;
}

/**
 * Resolves the configured background source into a scene-space oriented box.
 */
static bool try_resolve_background_scene_box(obs_source_t *root_scene_source,
	obs_source_t *background_source, BackgroundSceneBox &box)
{
	if (root_scene_source == nullptr || background_source == nullptr) {
		return false;
	}

	RootSceneItemMatch match = {};
	if (search_root_scene_item(root_scene_source, background_source, match) != SceneSearchStatus::Unique) {
		return false;
	}

	return build_background_scene_box(match, box);
}

/**
 * Resolves the configured background source into source-local to scene UV mapping.
 */
static bool try_resolve_background_scene_uv_mapping(obs_source_t *root_scene_source,
	obs_source_t *background_source, uint32_t local_width, uint32_t local_height,
	SceneUvMapping &mapping)
{
	if (root_scene_source == nullptr || background_source == nullptr || local_width == 0 ||
	    local_height == 0) {
		return false;
	}

	RootSceneItemMatch match = {};
	if (search_root_scene_item(root_scene_source, background_source, match) !=
	    SceneSearchStatus::Unique) {
		return false;
	}

	build_scene_uv_mapping(match, local_width, local_height, mapping);
	return true;
}

/**
 * Populates a list property with available sources.
 */
static bool enum_sources_for_property(void *data, obs_source_t *source)
{
auto *property = static_cast<obs_property_t *>(data);
const char *name = obs_source_get_name(source);
obs_property_list_add_string(property, name, name);
return true;
}

/**
 * Returns human readable filter display name.
 */
static const char *scene_lighting_get_name(void *)
{
	return obs_module_text(kTextFilterName);
}

/**
 * Applies persisted settings to in-memory filter state.
 */
static void scene_lighting_update(void *data, obs_data_t *settings)
{
auto *filter = static_cast<SceneLightingFilter *>(data);
	set_background_source_name(filter, obs_data_get_string(settings, kPropBackgroundSource));
	filter->blend_mode = static_cast<BlendMode>(clamp_int(
		static_cast<int32_t>(obs_data_get_int(settings, kPropBlendMode)), 0, 3));
	filter->intensity_percent = clamp_int(static_cast<int32_t>(obs_data_get_int(settings, kPropIntensity)),
		0, 100);
	filter->blur_radius = clamp_int(static_cast<int32_t>(obs_data_get_int(settings, kPropBlurRadius)), 1, 100);
	filter->edge_width = clamp_int(static_cast<int32_t>(obs_data_get_int(settings, kPropEdgeWidth)), 1,
		static_cast<int32_t>(kMaxEdgeWidth));
	filter->edge_threshold = clamp_int(
		static_cast<int32_t>(obs_data_get_int(settings, kPropEdgeThreshold)), 1, 255);
filter->enable_wrap = obs_data_get_bool(settings, kPropEnableWrap);
filter->enable_rim = obs_data_get_bool(settings, kPropEnableRim);
	filter->rim_angle = clamp_int(static_cast<int32_t>(obs_data_get_int(settings, kPropRimAngle)),
		-180, 180);
	filter->tint_color = static_cast<uint32_t>(obs_data_get_int(settings, kPropTintColor));
	filter->sampling_method = static_cast<SamplingMode>(clamp_int(
		static_cast<int32_t>(obs_data_get_int(settings, kPropSamplingMethod)), 0, 1));
update_background_source(filter);
}

/**
 * Lazily creates graphics resources in the render path.
 */
static bool ensure_graphics_resources(SceneLightingFilter *filter)
{
	if (filter->effect != nullptr && filter->background_render_target != nullptr) {
		return true;
	}

	if (filter->effect == nullptr) {
		const char *effect_file = "scene_lighting.effect";
		char *effect_path = obs_module_file(effect_file);
		char *effect_error = nullptr;
		filter->effect = gs_effect_create_from_file(effect_path, &effect_error);
		if (filter->effect == nullptr) {
			blog(LOG_ERROR, "[scene-lighting][resources] failed to load effect from '%s'%s%s",
				effect_path != nullptr ? effect_path : "<null>",
				effect_error != nullptr ? ": " : "",
				effect_error != nullptr ? effect_error : "");
		}
		if (effect_error != nullptr) {
			bfree(effect_error);
		}
		bfree(effect_path);
	}

	if (filter->background_render_target == nullptr) {
		filter->background_render_target = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
		if (filter->background_render_target == nullptr) {
			blog(LOG_ERROR,
				"[scene-lighting][resources] failed to create background render target");
		}
	}

	return filter->effect != nullptr && filter->background_render_target != nullptr;
}

/**
 * Creates filter state and initializes graphics resources.
 */
static void *scene_lighting_create(obs_data_t *settings, obs_source_t *source)
{
auto *filter = new SceneLightingFilter{};
filter->source = source;

scene_lighting_update(filter, settings);
return filter;
}

/**
 * Destroys filter state and releases resources.
 */
static void scene_lighting_destroy(void *data)
{
auto *filter = static_cast<SceneLightingFilter *>(data);

if (filter->background_source != nullptr) {
obs_weak_source_release(filter->background_source);
}
bfree(filter->background_source_name);

obs_enter_graphics();
if (filter->effect != nullptr) {
gs_effect_destroy(filter->effect);
}
if (filter->background_render_target != nullptr) {
gs_texrender_destroy(filter->background_render_target);
}
obs_leave_graphics();

delete filter;
}

/**
 * Writes default settings for newly added filters.
 */
static void scene_lighting_defaults(obs_data_t *settings)
{
obs_data_set_default_string(settings, kPropBackgroundSource, "");
obs_data_set_default_int(settings, kPropBlendMode, static_cast<int>(BlendMode::Screen));
obs_data_set_default_int(settings, kPropIntensity, 50);
obs_data_set_default_int(settings, kPropBlurRadius, 15);
obs_data_set_default_int(settings, kPropEdgeWidth, 10);
obs_data_set_default_int(settings, kPropEdgeThreshold, 16);
obs_data_set_default_bool(settings, kPropEnableWrap, true);
obs_data_set_default_bool(settings, kPropEnableRim, true);
obs_data_set_default_int(settings, kPropRimAngle, 45);
obs_data_set_default_int(settings, kPropTintColor, 0xFFFFFF);
obs_data_set_default_int(settings, kPropSamplingMethod, static_cast<int>(SamplingMode::Average));
}

/**
 * Creates filter properties presented in OBS UI.
 */
static obs_properties_t *scene_lighting_properties(void *data)
{
	UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();

	obs_property_t *source_prop = obs_properties_add_list(props, kPropBackgroundSource,
		obs_module_text(kTextBackgroundSource),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(source_prop, obs_module_text(kTextNone), "");
	obs_enum_sources(enum_sources_for_property, source_prop);
	obs_enum_scenes(enum_sources_for_property, source_prop);

	obs_property_t *blend_mode = obs_properties_add_list(props, kPropBlendMode,
		obs_module_text(kTextBlendMode),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(blend_mode, obs_module_text(kTextBlendMultiply),
		static_cast<int>(BlendMode::Multiply));
	obs_property_list_add_int(blend_mode, obs_module_text(kTextBlendScreen),
		static_cast<int>(BlendMode::Screen));
	obs_property_list_add_int(blend_mode, obs_module_text(kTextBlendOverlay),
		static_cast<int>(BlendMode::Overlay));
	obs_property_list_add_int(blend_mode, obs_module_text(kTextBlendAdd), static_cast<int>(BlendMode::Add));

	obs_properties_add_int_slider(props, kPropIntensity, obs_module_text(kTextIntensity), 0, 100, 1);
	obs_properties_add_int_slider(props, kPropBlurRadius, obs_module_text(kTextBlurRadius), 1, 100, 1);
	obs_properties_add_int_slider(props, kPropEdgeWidth, obs_module_text(kTextEdgeWidth), 1,
		static_cast<int>(kMaxEdgeWidth), 1);
	obs_properties_add_int_slider(props, kPropEdgeThreshold, obs_module_text(kTextEdgeThreshold), 1, 255, 1);
	obs_properties_add_bool(props, kPropEnableWrap, obs_module_text(kTextLightWrap));
	obs_properties_add_bool(props, kPropEnableRim, obs_module_text(kTextRimLight));
	obs_properties_add_int_slider(props, kPropRimAngle, obs_module_text(kTextRimAngle), -180, 180, 1);
	obs_properties_add_color(props, kPropTintColor, obs_module_text(kTextTintColor));

	obs_property_t *sampling = obs_properties_add_list(props, kPropSamplingMethod,
		obs_module_text(kTextSamplingMethod),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(sampling, obs_module_text(kTextSamplingAverage),
		static_cast<int>(SamplingMode::Average));
	obs_property_list_add_int(sampling, obs_module_text(kTextSamplingCenter),
		static_cast<int>(SamplingMode::CenterPoints));

return props;
}

/**
 * Renders selected background source into intermediate texture.
 */
static gs_texture_t *render_background(SceneLightingFilter *filter, uint32_t width, uint32_t height,
	obs_source_t *excluded_source)
{
if (filter->background_source == nullptr || filter->background_render_target == nullptr || width == 0 ||
    height == 0) {
return nullptr;
}

if (filter->rendering_background) {
return nullptr;
}

	obs_source_t *background = obs_weak_source_get_source(filter->background_source);
	if (background == nullptr || background == excluded_source) {
		if (background != nullptr) {
			obs_source_release(background);
		}
		return nullptr;
	}

gs_texrender_reset(filter->background_render_target);
if (!gs_texrender_begin(filter->background_render_target, width, height)) {
obs_source_release(background);
return nullptr;
}

	const struct vec4 clear_color = {0.0F, 0.0F, 0.0F, 0.0F};
	gs_viewport_push();
	gs_projection_push();
	gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0F, 0);
	gs_ortho(0.0F, static_cast<float>(width), 0.0F, static_cast<float>(height), -100.0F, 100.0F);
	filter->rendering_background = true;
	obs_source_video_render(background);
	filter->rendering_background = false;
	gs_projection_pop();
	gs_viewport_pop();
	gs_texrender_end(filter->background_render_target);

obs_source_release(background);
return gs_texrender_get_texture(filter->background_render_target);
}

/**
 * Updates effect uniforms and renders the filtered output.
 */
static void scene_lighting_render(void *data, gs_effect_t *)
{
auto *filter = static_cast<SceneLightingFilter *>(data);

	if (filter->rendering_background) {
		obs_source_skip_video_filter(filter->source);
		return;
	}

	obs_source_t *parent = obs_filter_get_parent(filter->source);
obs_source_t *target = obs_filter_get_target(filter->source);
if (target == nullptr) {
obs_source_skip_video_filter(filter->source);
return;
}

	if (!ensure_graphics_resources(filter)) {
		obs_source_skip_video_filter(filter->source);
		return;
	}

const uint32_t width = obs_source_get_base_width(target);
const uint32_t height = obs_source_get_base_height(target);
if (width == 0 || height == 0) {
obs_source_skip_video_filter(filter->source);
return;
}

	SceneUvMapping scene_mapping = {};
	SceneUvMapping background_mapping = {};
	BackgroundSceneBox background_box = {};
	obs_source_t *resolved_root_scene = nullptr;
	try_resolve_scene_uv_mapping(parent, width, height, scene_mapping, &resolved_root_scene);
	const uint32_t scene_width = scene_mapping.available ? scene_mapping.background_width : width;
	const uint32_t scene_height = scene_mapping.available ? scene_mapping.background_height : height;
	uint32_t background_source_width = 0;
	uint32_t background_source_height = 0;
	obs_source_t *background_source = nullptr;
	if (scene_mapping.available && filter->background_source != nullptr) {
		background_source = obs_weak_source_get_source(filter->background_source);
		if (background_source != nullptr &&
		    background_source != (parent != nullptr ? parent : target) && resolved_root_scene != nullptr) {
			background_source_width = obs_source_get_base_width(background_source);
			background_source_height = obs_source_get_base_height(background_source);
			if (background_source_width == 0 || background_source_height == 0) {
				background_source_width = obs_source_get_width(background_source);
				background_source_height = obs_source_get_height(background_source);
			}
			if (background_source_width != 0 && background_source_height != 0) {
				try_resolve_background_scene_uv_mapping(resolved_root_scene, background_source,
					background_source_width, background_source_height, background_mapping);
			}
			try_resolve_background_scene_box(resolved_root_scene, background_source, background_box);
		}
	}
	if (background_source != nullptr) {
		obs_source_release(background_source);
	}
	if (resolved_root_scene != nullptr) {
		obs_source_release(resolved_root_scene);
	}
	const uint32_t background_width = scene_width;
	const uint32_t background_height = scene_height;
	gs_texture_t *background_texture =
		render_background(filter, background_width, background_height, parent != nullptr ? parent : target);

if (!obs_source_process_filter_begin(filter->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
obs_source_skip_video_filter(filter->source);
return;
}

const float intensity = static_cast<float>(filter->intensity_percent) / 100.0F;
const float edge_threshold = static_cast<float>(filter->edge_threshold) / 255.0F;
	const float rim_angle_rad = static_cast<float>(filter->rim_angle) * (kPi / 180.0F);
	const struct vec2 light_dir = {std::cos(rim_angle_rad), -std::sin(rim_angle_rad)};
	const struct vec2 foreground_texel_size = {1.0F / static_cast<float>(width),
		1.0F / static_cast<float>(height)};
	const struct vec2 background_texel_size = {1.0F / static_cast<float>(background_width),
		1.0F / static_cast<float>(background_height)};
	struct vec2 background_uv_scale = {1.0F, 1.0F};
	if (background_source_width != 0 && background_source_height != 0) {
		background_uv_scale = {static_cast<float>(background_source_width) /
			static_cast<float>(background_width),
			static_cast<float>(background_source_height) / static_cast<float>(background_height)};
	}
float tint_r = 1.0F;
float tint_g = 1.0F;
float tint_b = 1.0F;
decode_tint(filter->tint_color, tint_r, tint_g, tint_b);
	const struct vec3 tint_color = {tint_r, tint_g, tint_b};

gs_eparam_t *param_background_tex = gs_effect_get_param_by_name(filter->effect, "background_tex");
gs_eparam_t *param_intensity = gs_effect_get_param_by_name(filter->effect, "intensity");
gs_eparam_t *param_edge_width = gs_effect_get_param_by_name(filter->effect, "edge_width");
gs_eparam_t *param_edge_threshold = gs_effect_get_param_by_name(filter->effect, "edge_threshold");
gs_eparam_t *param_blend_mode = gs_effect_get_param_by_name(filter->effect, "blend_mode");
gs_eparam_t *param_enable_wrap = gs_effect_get_param_by_name(filter->effect, "enable_wrap");
gs_eparam_t *param_enable_rim = gs_effect_get_param_by_name(filter->effect, "enable_rim");
gs_eparam_t *param_light_dir = gs_effect_get_param_by_name(filter->effect, "light_dir");
gs_eparam_t *param_tint_color = gs_effect_get_param_by_name(filter->effect, "tint_color");
	gs_eparam_t *param_sampling_method = gs_effect_get_param_by_name(filter->effect, "sampling_method");
	gs_eparam_t *param_blur_radius = gs_effect_get_param_by_name(filter->effect, "blur_radius");
	gs_eparam_t *param_background_available = gs_effect_get_param_by_name(filter->effect, "background_available");
	gs_eparam_t *param_foreground_texel_size = gs_effect_get_param_by_name(filter->effect,
		"foreground_texel_size");
	gs_eparam_t *param_background_texel_size = gs_effect_get_param_by_name(filter->effect,
		"background_texel_size");
	gs_eparam_t *param_scene_uv_origin = gs_effect_get_param_by_name(filter->effect, "scene_uv_origin");
	gs_eparam_t *param_scene_uv_x = gs_effect_get_param_by_name(filter->effect, "scene_uv_x");
	gs_eparam_t *param_scene_uv_y = gs_effect_get_param_by_name(filter->effect, "scene_uv_y");
	gs_eparam_t *param_scene_mapping_available = gs_effect_get_param_by_name(filter->effect,
		"scene_mapping_available");
	gs_eparam_t *param_background_scene_uv_origin = gs_effect_get_param_by_name(filter->effect,
		"background_scene_uv_origin");
	gs_eparam_t *param_background_scene_uv_x = gs_effect_get_param_by_name(filter->effect,
		"background_scene_uv_x");
	gs_eparam_t *param_background_scene_uv_y = gs_effect_get_param_by_name(filter->effect,
		"background_scene_uv_y");
	gs_eparam_t *param_background_scene_mapping_available = gs_effect_get_param_by_name(filter->effect,
		"background_scene_mapping_available");
	gs_eparam_t *param_background_render_uv_scale = gs_effect_get_param_by_name(filter->effect,
		"background_render_uv_scale");
	gs_eparam_t *param_background_box_center = gs_effect_get_param_by_name(filter->effect,
		"background_box_center");
	gs_eparam_t *param_background_box_axis_x = gs_effect_get_param_by_name(filter->effect,
		"background_box_axis_x");
	gs_eparam_t *param_background_box_axis_y = gs_effect_get_param_by_name(filter->effect,
		"background_box_axis_y");
	gs_eparam_t *param_background_box_half_extent = gs_effect_get_param_by_name(filter->effect,
		"background_box_half_extent");
	gs_eparam_t *param_background_box_available = gs_effect_get_param_by_name(filter->effect,
		"background_box_available");

if (param_background_tex != nullptr) {
gs_effect_set_texture(param_background_tex, background_texture);
}
if (param_intensity != nullptr) {
gs_effect_set_float(param_intensity, intensity);
}
if (param_edge_width != nullptr) {
gs_effect_set_float(param_edge_width, static_cast<float>(filter->edge_width));
}
if (param_edge_threshold != nullptr) {
gs_effect_set_float(param_edge_threshold, edge_threshold);
}
if (param_blend_mode != nullptr) {
gs_effect_set_float(param_blend_mode, static_cast<float>(static_cast<int>(filter->blend_mode)));
}
if (param_enable_wrap != nullptr) {
gs_effect_set_float(param_enable_wrap, filter->enable_wrap ? 1.0F : 0.0F);
}
if (param_enable_rim != nullptr) {
gs_effect_set_float(param_enable_rim, filter->enable_rim ? 1.0F : 0.0F);
}
if (param_light_dir != nullptr) {
gs_effect_set_vec2(param_light_dir, &light_dir);
}
if (param_tint_color != nullptr) {
gs_effect_set_vec3(param_tint_color, &tint_color);
}
if (param_sampling_method != nullptr) {
gs_effect_set_float(param_sampling_method,
static_cast<float>(static_cast<int>(filter->sampling_method)));
}
if (param_blur_radius != nullptr) {
gs_effect_set_float(param_blur_radius, static_cast<float>(filter->blur_radius));
}
if (param_background_available != nullptr) {
gs_effect_set_float(param_background_available, background_texture != nullptr ? 1.0F : 0.0F);
}
	if (param_foreground_texel_size != nullptr) {
		gs_effect_set_vec2(param_foreground_texel_size, &foreground_texel_size);
	}
	if (param_background_texel_size != nullptr) {
		gs_effect_set_vec2(param_background_texel_size, &background_texel_size);
	}
	if (param_scene_uv_origin != nullptr) {
		gs_effect_set_vec2(param_scene_uv_origin, &scene_mapping.origin);
	}
	if (param_scene_uv_x != nullptr) {
		gs_effect_set_vec2(param_scene_uv_x, &scene_mapping.axis_x);
	}
	if (param_scene_uv_y != nullptr) {
		gs_effect_set_vec2(param_scene_uv_y, &scene_mapping.axis_y);
	}
	if (param_scene_mapping_available != nullptr) {
		gs_effect_set_float(param_scene_mapping_available, scene_mapping.available ? 1.0F : 0.0F);
	}
	if (param_background_scene_uv_origin != nullptr) {
		gs_effect_set_vec2(param_background_scene_uv_origin, &background_mapping.origin);
	}
	if (param_background_scene_uv_x != nullptr) {
		gs_effect_set_vec2(param_background_scene_uv_x, &background_mapping.axis_x);
	}
	if (param_background_scene_uv_y != nullptr) {
		gs_effect_set_vec2(param_background_scene_uv_y, &background_mapping.axis_y);
	}
	if (param_background_scene_mapping_available != nullptr) {
		gs_effect_set_float(param_background_scene_mapping_available,
			background_mapping.available ? 1.0F : 0.0F);
	}
	if (param_background_render_uv_scale != nullptr) {
		gs_effect_set_vec2(param_background_render_uv_scale, &background_uv_scale);
	}
	if (param_background_box_center != nullptr) {
		gs_effect_set_vec2(param_background_box_center, &background_box.center);
	}
	if (param_background_box_axis_x != nullptr) {
		gs_effect_set_vec2(param_background_box_axis_x, &background_box.axis_x);
	}
	if (param_background_box_axis_y != nullptr) {
		gs_effect_set_vec2(param_background_box_axis_y, &background_box.axis_y);
	}
	if (param_background_box_half_extent != nullptr) {
		gs_effect_set_vec2(param_background_box_half_extent, &background_box.half_extent);
	}
	if (param_background_box_available != nullptr) {
		gs_effect_set_float(param_background_box_available, background_box.available ? 1.0F : 0.0F);
	}

obs_source_process_filter_end(filter->source, filter->effect, width, height);
}

/**
 * Static source info declaration used to register the filter.
 */
static obs_source_info scene_lighting_filter_info = {};

/**
 * Module load callback that registers filter source info.
 */
bool obs_module_load(void)
{
	scene_lighting_filter_info.id = kFilterId;
	scene_lighting_filter_info.type = OBS_SOURCE_TYPE_FILTER;
	scene_lighting_filter_info.output_flags = OBS_SOURCE_VIDEO;
	scene_lighting_filter_info.get_name = scene_lighting_get_name;
	scene_lighting_filter_info.create = scene_lighting_create;
	scene_lighting_filter_info.destroy = scene_lighting_destroy;
	scene_lighting_filter_info.update = scene_lighting_update;
	scene_lighting_filter_info.video_render = scene_lighting_render;
	scene_lighting_filter_info.get_defaults = scene_lighting_defaults;
	scene_lighting_filter_info.get_properties = scene_lighting_properties;
	obs_register_source(&scene_lighting_filter_info);
	blog(LOG_INFO, "Loaded scene lighting filter plugin");
	return true;
}
