#include <obs-module.h>
#include <graphics/graphics.h>
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
/** OBS source context for this filter. */
obs_source_t *source = nullptr;
/** Name of the configured background source. */
char *background_source_name = nullptr;
/** Weak reference to configured background source. */
obs_weak_source_t *background_source = nullptr;
/** Loaded graphics effect. */
gs_effect_t *effect = nullptr;
/** Offscreen render target for background source. */
gs_texrender_t *background_render_target = nullptr;
/** Configured blend mode. */
BlendMode blend_mode = BlendMode::Screen;
/** Effect intensity in percent. */
int32_t intensity_percent = 50;
/** Sampling blur radius in pixels. */
int32_t blur_radius = 15;
/** Edge width in pixels. */
int32_t edge_width = 10;
/** Edge alpha threshold in byte units. */
int32_t edge_threshold = 16;
/** Whether light wrap is enabled. */
bool enable_wrap = true;
/** Whether rim lighting is enabled. */
bool enable_rim = true;
/** Rim light direction angle in degrees. */
int32_t rim_angle = 45;
/** RGB tint color packed into BGR integer. */
uint32_t tint_color = 0xFFFFFF;
/** Sampling method for ambient extraction. */
SamplingMode sampling_method = SamplingMode::Average;
};

/**
 * Returns plugin description shown in OBS UI.
 */
MODULE_EXPORT const char *obs_module_description(void)
{
return "Applies scene lighting from a selected background source to the filtered source.";
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
r = static_cast<float>((tint_color >> 16U) & 0xFFU) * normalize;
g = static_cast<float>((tint_color >> 8U) & 0xFFU) * normalize;
b = static_cast<float>(tint_color & 0xFFU) * normalize;
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
 * Creates filter state and initializes graphics resources.
 */
static void *scene_lighting_create(obs_data_t *settings, obs_source_t *source)
{
auto *filter = new SceneLightingFilter{};
filter->source = source;

char *effect_path = obs_module_file("scene_lighting.effect");
obs_enter_graphics();
filter->effect = gs_effect_create_from_file(effect_path, nullptr);
filter->background_render_target = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
obs_leave_graphics();
bfree(effect_path);

scene_lighting_update(filter, settings);
return filter;
}

/**
 * Destroys filter state and releases resources.
 */
static void scene_lighting_destroy(void *data)
{
auto *filter = static_cast<SceneLightingFilter *>(data);

obs_weak_source_release(filter->background_source);
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
OBS_UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();

	obs_property_t *source_prop = obs_properties_add_list(props, kPropBackgroundSource,
		obs_module_text(kTextBackgroundSource),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(source_prop, obs_module_text(kTextNone), "");
	obs_enum_sources(enum_sources_for_property, source_prop);

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
	obs_source_t *target)
{
if (filter->background_source == nullptr || filter->background_render_target == nullptr || width == 0 ||
    height == 0) {
return nullptr;
}

	obs_source_t *background = obs_weak_source_get_source(filter->background_source);
	if (background == nullptr || background == target) {
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
	obs_source_video_render(background);
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
obs_source_t *target = obs_filter_get_target(filter->source);
if (target == nullptr || filter->effect == nullptr) {
obs_source_skip_video_filter(filter->source);
return;
}

const uint32_t width = obs_source_get_base_width(target);
const uint32_t height = obs_source_get_base_height(target);
if (width == 0 || height == 0) {
obs_source_skip_video_filter(filter->source);
return;
}

gs_texture_t *background_texture = render_background(filter, width, height, target);

if (!obs_source_process_filter_begin(filter->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
return;
}

const float intensity = static_cast<float>(filter->intensity_percent) / 100.0F;
const float edge_threshold = static_cast<float>(filter->edge_threshold) / 255.0F;
	const float rim_angle_rad = static_cast<float>(filter->rim_angle) * (kPi / 180.0F);
	const struct vec2 light_dir = {std::cos(rim_angle_rad), -std::sin(rim_angle_rad)};
	const struct vec2 texel_size = {1.0F / static_cast<float>(width), 1.0F / static_cast<float>(height)};
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
	gs_eparam_t *param_texel_size = gs_effect_get_param_by_name(filter->effect, "texel_size");
	gs_eparam_t *param_max_edge_width = gs_effect_get_param_by_name(filter->effect, "max_edge_width");

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
	if (param_texel_size != nullptr) {
		gs_effect_set_vec2(param_texel_size, &texel_size);
	}
	if (param_max_edge_width != nullptr) {
		gs_effect_set_float(param_max_edge_width, kMaxEdgeWidth);
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
