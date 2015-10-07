/*
** Copyright 2012 Intel Corporation
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#undef  CPF_HAL_TAG_BEGIN
#undef  CPF_HAL_TAG
#undef  CPF_HAL_TAG_END
#define CPF_HAL_TAG_BEGIN
#define CPF_HAL_TAG(tagname)
#define CPF_HAL_TAG_END

#ifndef STRINGS_NOT_ENUMS
#ifndef __CPF_HAL_ENUMS__
#define __CPF_HAL_ENUMS__

#include <stdint.h>  /* defines integer types with specified widths */

typedef struct
{
    uint32_t data_offset;
    uint32_t table_offset;
    uint32_t string_offset;
    uint32_t tags_count;
    uint32_t tags_min;
    uint32_t tags_max;
    uint32_t flags;
} cpf_hal_header_t;

typedef enum
{
    sparse_en =  (1 << 0)
} cpf_hal_flags_t;

typedef enum
{
    tag_unused = (1 << 31),
    tag_table  = (1 << 30),
    tag_string = (1 << 29),
    tag_value  = (1 << 28),
    tag_bool   = (1 << 27),
    tag_fpoint = (1 << 26),
    tag_float  = (1 << 25)  // Never present in HALB records
} cpf_hal_tagtype_t;

#undef  CPF_HAL_TAG_BEGIN
#undef  CPF_HAL_TAG
#undef  CPF_HAL_TAG_END
#define CPF_HAL_TAG_BEGIN typedef enum {
#define CPF_HAL_TAG(tagname) tagname,
#define CPF_HAL_TAG_END CPF_HAL_TAG(TotalTagCount) } cpf_hal_tag_t;
#endif // __CPF_HAL_ENUMS__
#endif // STRINGS_NOT_ENUMS

#ifdef STRINGS_NOT_ENUMS
#ifndef __CPF_HAL_STRINGS__
#define __CPF_HAL_STRINGS__
#undef  CPF_HAL_TAG_BEGIN
#undef  CPF_HAL_TAG
#undef  CPF_HAL_TAG_END
#define CPF_HAL_TAG_BEGIN char *cpf_hal_tag_names_t[] = {
#define CPF_HAL_TAG(tagname) #tagname,
#define CPF_HAL_TAG_END };
#endif // __CPF_HAL_STRINGS__
#endif // STRINGS_NOT_ENUMS

#ifdef __cplusplus
namespace CPF {
#endif

CPF_HAL_TAG_BEGIN
/*
** These tags are reserved for generic purpose use in property tables
** related to some particular tag. That is, we can neatly attach
** number of "common" qualities like "min", "max" or "default" values
** to some feature or setting, like exposure or gain, by storing the
** respective values in a table related to that feature or setting,
** and so we don't have to have separate tags for each "propertymin"
** and "propertymax".
*/
CPF_HAL_TAG     ( Any )
CPF_HAL_TAG     ( Name )
CPF_HAL_TAG     ( Min )
CPF_HAL_TAG     ( Max )
CPF_HAL_TAG     ( Step )
CPF_HAL_TAG     ( Default )
CPF_HAL_TAG     ( Top )
CPF_HAL_TAG     ( Bottom )
CPF_HAL_TAG     ( Left )
CPF_HAL_TAG     ( Right )
CPF_HAL_TAG     ( Up )
CPF_HAL_TAG     ( Down )
CPF_HAL_TAG     ( Width )
CPF_HAL_TAG     ( Height )
CPF_HAL_TAG     ( Horizontal )
CPF_HAL_TAG     ( Vertical )
CPF_HAL_TAG     ( Weight )
CPF_HAL_TAG     ( Enable )
CPF_HAL_TAG     ( Disable )
CPF_HAL_TAG     ( Threshold )
CPF_HAL_TAG     ( Lag )
CPF_HAL_TAG     ( InitialSkip )
CPF_HAL_TAG     ( WeightGrid )
CPF_HAL_TAG     ( Size )
CPF_HAL_TAG     ( Data )
CPF_HAL_TAG     ( Reserved25 )
CPF_HAL_TAG     ( Reserved26 )
CPF_HAL_TAG     ( Reserved27 )
CPF_HAL_TAG     ( Reserved28 )
CPF_HAL_TAG     ( Reserved29 )
CPF_HAL_TAG     ( Reserved30 )
CPF_HAL_TAG     ( Reserved31 )
CPF_HAL_TAG     ( Reserved32 )
CPF_HAL_TAG     ( Reserved33 )
CPF_HAL_TAG     ( Reserved34 )
CPF_HAL_TAG     ( Reserved35 )
CPF_HAL_TAG     ( Reserved36 )
CPF_HAL_TAG     ( Reserved37 )
CPF_HAL_TAG     ( Reserved38 )
CPF_HAL_TAG     ( Reserved39 )
CPF_HAL_TAG     ( Reserved40 )
CPF_HAL_TAG     ( Reserved41 )
CPF_HAL_TAG     ( Reserved42 )
CPF_HAL_TAG     ( Reserved43 )
CPF_HAL_TAG     ( Reserved44 )
CPF_HAL_TAG     ( Reserved45 )
CPF_HAL_TAG     ( Reserved46 )
CPF_HAL_TAG     ( Reserved47 )
CPF_HAL_TAG     ( Reserved48 )
CPF_HAL_TAG     ( Reserved49 )
/*
** Tags for features and settings that may bear some dependency
** to product, platform or OS using the camera module. In order
** to make it easier to reutilize CPF configuration files across
** platforms and OS's sharing the same camera module, the use of
** these tags should likely be minimized.
*/
CPF_HAL_TAG     ( Ull )          /* Ultra-low-light properties */
CPF_HAL_TAG     ( ZoomDigital )  /* Zoom properties */
CPF_HAL_TAG     ( IspVamemType ) /* Represents ia_css_vamem_type */
CPF_HAL_TAG     ( Reserved53 )
CPF_HAL_TAG     ( Reserved54 )
CPF_HAL_TAG     ( Reserved55 )
CPF_HAL_TAG     ( Reserved56 )
CPF_HAL_TAG     ( Reserved57 )
CPF_HAL_TAG     ( Reserved58 )
CPF_HAL_TAG     ( Reserved59 )
CPF_HAL_TAG     ( Reserved60 )
CPF_HAL_TAG     ( Reserved61 )
CPF_HAL_TAG     ( Reserved62 )
CPF_HAL_TAG     ( Reserved63 )
CPF_HAL_TAG     ( Reserved64 )
CPF_HAL_TAG     ( Reserved65 )
CPF_HAL_TAG     ( Reserved66 )
CPF_HAL_TAG     ( Reserved67 )
CPF_HAL_TAG     ( Reserved68 )
CPF_HAL_TAG     ( Reserved69 )
CPF_HAL_TAG     ( Reserved70 )
CPF_HAL_TAG     ( Reserved71 )
CPF_HAL_TAG     ( Reserved72 )
CPF_HAL_TAG     ( Reserved73 )
CPF_HAL_TAG     ( Reserved74 )
CPF_HAL_TAG     ( Reserved75 )
CPF_HAL_TAG     ( Reserved76 )
CPF_HAL_TAG     ( Reserved77 )
CPF_HAL_TAG     ( Reserved78 )
CPF_HAL_TAG     ( Reserved79 )
CPF_HAL_TAG     ( Reserved80 )
CPF_HAL_TAG     ( Reserved81 )
CPF_HAL_TAG     ( Reserved82 )
CPF_HAL_TAG     ( Reserved83 )
CPF_HAL_TAG     ( Reserved84 )
CPF_HAL_TAG     ( Reserved85 )
CPF_HAL_TAG     ( Reserved86 )
CPF_HAL_TAG     ( Reserved87 )
CPF_HAL_TAG     ( Reserved88 )
CPF_HAL_TAG     ( Reserved89 )
CPF_HAL_TAG     ( Reserved90 )
CPF_HAL_TAG     ( Reserved91 )
CPF_HAL_TAG     ( Reserved92 )
CPF_HAL_TAG     ( Reserved93 )
CPF_HAL_TAG     ( Reserved94 )
CPF_HAL_TAG     ( Reserved95 )
CPF_HAL_TAG     ( Reserved96 )
CPF_HAL_TAG     ( Reserved97 )
CPF_HAL_TAG     ( Reserved98 )
CPF_HAL_TAG     ( Reserved99 )
/*
** Here we have tags for features and settings that only relate to
** camera module in question, not to product, platform or OS
** using the module. The use of these tags is encouraged, as
** they provide maximally reusable configuration for a particular
** camera module.
*/
CPF_HAL_TAG     ( NeedsIsp )     /* true = ISP needed, false = inbuilt ISP */
CPF_HAL_TAG     ( Needs3A )      /* true = 3A needed, false = inbuilt 3A */
CPF_HAL_TAG     ( HasFlash )     /* true = flash present, false = no flash light */
CPF_HAL_TAG     ( SizeActive )   /* Max resolution of sensor pixel array */
CPF_HAL_TAG     ( Exposure )     /* Exposure properties */
CPF_HAL_TAG     ( Gain )         /* Gain properties */
CPF_HAL_TAG     ( Fov )          /* Field of View if fixed */
CPF_HAL_TAG     ( Aperture )     /* Aperture if fixed, F-number */
CPF_HAL_TAG     ( Statistics )   /* Number of statistics to skip initially */
CPF_HAL_TAG     ( ExposureWeights )  /*exposure weight TAG*/
CPF_HAL_TAG     ( EmdHeadFile )  /* Define embeded metadata decoder head file for SOC sensor */
CPF_HAL_TAG     ( SbsMetadata )  /* true = side-by-side metadata including two metadata each event */
CPF_HAL_TAG     ( Reserved112 )
CPF_HAL_TAG     ( Reserved113 )
CPF_HAL_TAG     ( Reserved114 )
CPF_HAL_TAG     ( Reserved115 )
CPF_HAL_TAG     ( Reserved116 )
CPF_HAL_TAG     ( Reserved117 )
CPF_HAL_TAG     ( Reserved118 )
CPF_HAL_TAG     ( Reserved119 )
CPF_HAL_TAG     ( Reserved120 )
CPF_HAL_TAG     ( Reserved121 )
CPF_HAL_TAG     ( Reserved122 )
CPF_HAL_TAG     ( Reserved123 )
CPF_HAL_TAG     ( Reserved124 )
CPF_HAL_TAG     ( Reserved125 )
CPF_HAL_TAG     ( Reserved126 )
CPF_HAL_TAG     ( Reserved127 )
CPF_HAL_TAG     ( Reserved128 )
CPF_HAL_TAG     ( Reserved129 )
CPF_HAL_TAG     ( Reserved130 )
CPF_HAL_TAG     ( Reserved131 )
CPF_HAL_TAG     ( Reserved132 )
CPF_HAL_TAG     ( Reserved133 )
CPF_HAL_TAG     ( Reserved134 )
CPF_HAL_TAG     ( Reserved135 )
CPF_HAL_TAG     ( Reserved136 )
CPF_HAL_TAG     ( Reserved137 )
CPF_HAL_TAG     ( Reserved138 )
CPF_HAL_TAG     ( Reserved139 )
CPF_HAL_TAG     ( Reserved140 )
CPF_HAL_TAG     ( Reserved141 )
CPF_HAL_TAG     ( Reserved142 )
CPF_HAL_TAG     ( Reserved143 )
CPF_HAL_TAG     ( Reserved144 )
CPF_HAL_TAG     ( Reserved145 )
CPF_HAL_TAG     ( Reserved146 )
CPF_HAL_TAG     ( Reserved147 )
CPF_HAL_TAG     ( Reserved148 )
CPF_HAL_TAG     ( Reserved149 )
CPF_HAL_TAG     ( Reserved150 )
CPF_HAL_TAG     ( Reserved151 )
CPF_HAL_TAG     ( Reserved152 )
CPF_HAL_TAG     ( Reserved153 )
CPF_HAL_TAG     ( Reserved154 )
CPF_HAL_TAG     ( Reserved155 )
CPF_HAL_TAG     ( Reserved156 )
CPF_HAL_TAG     ( Reserved157 )
CPF_HAL_TAG     ( Reserved158 )
CPF_HAL_TAG     ( Reserved159 )
CPF_HAL_TAG     ( Reserved160 )
CPF_HAL_TAG     ( Reserved161 )
CPF_HAL_TAG     ( Reserved162 )
CPF_HAL_TAG     ( Reserved163 )
CPF_HAL_TAG     ( Reserved164 )
CPF_HAL_TAG     ( Reserved165 )
CPF_HAL_TAG     ( Reserved166 )
CPF_HAL_TAG     ( Reserved167 )
CPF_HAL_TAG     ( Reserved168 )
CPF_HAL_TAG     ( Reserved169 )
CPF_HAL_TAG     ( Reserved170 )
CPF_HAL_TAG     ( Reserved171 )
CPF_HAL_TAG     ( Reserved172 )
CPF_HAL_TAG     ( Reserved173 )
CPF_HAL_TAG     ( Reserved174 )
CPF_HAL_TAG     ( Reserved175 )
CPF_HAL_TAG     ( Reserved176 )
CPF_HAL_TAG     ( Reserved177 )
CPF_HAL_TAG     ( Reserved178 )
CPF_HAL_TAG     ( Reserved179 )
CPF_HAL_TAG     ( Reserved180 )
CPF_HAL_TAG     ( Reserved181 )
CPF_HAL_TAG     ( Reserved182 )
CPF_HAL_TAG     ( Reserved183 )
CPF_HAL_TAG     ( Reserved184 )
CPF_HAL_TAG     ( Reserved185 )
CPF_HAL_TAG     ( Reserved186 )
CPF_HAL_TAG     ( Reserved187 )
CPF_HAL_TAG     ( Reserved188 )
CPF_HAL_TAG     ( Reserved189 )
CPF_HAL_TAG     ( Reserved190 )
CPF_HAL_TAG     ( Reserved191 )
CPF_HAL_TAG     ( Reserved192 )
CPF_HAL_TAG     ( Reserved193 )
CPF_HAL_TAG     ( Reserved194 )
CPF_HAL_TAG     ( Reserved195 )
CPF_HAL_TAG     ( Reserved196 )
CPF_HAL_TAG     ( Reserved197 )
CPF_HAL_TAG     ( Reserved198 )
CPF_HAL_TAG     ( Reserved199 )
/*
** The following tags are attached to strings used by Android.
** Although easy to define, parsing the content of the strings later
** may turn out to be both difficult and inefficient, and also rules
** how these strings are constructed, may be vague. That is to say,
** we should prefer other means of defining the values and properties
** to make them more generic and less platform dependent
*/
CPF_HAL_TAG     ( EvMin )
CPF_HAL_TAG     ( EvMax )
CPF_HAL_TAG     ( EvStep )
CPF_HAL_TAG     ( EvDefault )
CPF_HAL_TAG     ( Saturations )
CPF_HAL_TAG     ( SaturationMin )
CPF_HAL_TAG     ( SaturationMax )
CPF_HAL_TAG     ( SaturationStep )
CPF_HAL_TAG     ( SaturationDefault )
CPF_HAL_TAG     ( Contrasts )
CPF_HAL_TAG     ( ContrastMin )
CPF_HAL_TAG     ( ContrastMax )
CPF_HAL_TAG     ( ContrastStep )
CPF_HAL_TAG     ( ContrastDefault )
CPF_HAL_TAG     ( Sharpnesses )
CPF_HAL_TAG     ( SharpnessMin )
CPF_HAL_TAG     ( SharpnessMax )
CPF_HAL_TAG     ( SharpnessStep )
CPF_HAL_TAG     ( SharpnessDefault )
CPF_HAL_TAG     ( SceneModes )
CPF_HAL_TAG     ( SceneModeDefault )
CPF_HAL_TAG     ( IsoModes )
CPF_HAL_TAG     ( IsoModeDefault )
CPF_HAL_TAG     ( AwbModes )
CPF_HAL_TAG     ( AwbModeDefault )
CPF_HAL_TAG     ( AeModes )
CPF_HAL_TAG     ( AeModeDefault )
CPF_HAL_TAG     ( FocusModes )
CPF_HAL_TAG     ( FocusModeDefault )
CPF_HAL_TAG     ( FlashModes )
CPF_HAL_TAG     ( FlashModeDefault )
CPF_HAL_TAG     ( EffectModes )
CPF_HAL_TAG     ( EffectModeDefault )
CPF_HAL_TAG     ( ExtendedEffectModes )
CPF_HAL_TAG     ( PreviewFpss )
CPF_HAL_TAG     ( PreviewFpsRanges )
CPF_HAL_TAG     ( PreviewFpsRangeDefault )
CPF_HAL_TAG     ( PreviewSizes )
CPF_HAL_TAG     ( PreviewSizeStillDefault )
CPF_HAL_TAG     ( PreviewSizeVideoDefault )
CPF_HAL_TAG     ( VideoSizes )
CPF_HAL_TAG     ( Reserved241 )
CPF_HAL_TAG     ( Reserved242 )
CPF_HAL_TAG     ( Reserved243 )
CPF_HAL_TAG     ( Reserved244 )
CPF_HAL_TAG     ( Reserved245 )
CPF_HAL_TAG     ( Reserved246 )
CPF_HAL_TAG     ( Reserved247 )
CPF_HAL_TAG     ( Reserved248 )
CPF_HAL_TAG     ( Reserved249 )
CPF_HAL_TAG     ( Reserved250 )
CPF_HAL_TAG     ( Reserved251 )
CPF_HAL_TAG     ( Reserved252 )
CPF_HAL_TAG     ( Reserved253 )
CPF_HAL_TAG     ( Reserved254 )
CPF_HAL_TAG     ( Reserved255 )
CPF_HAL_TAG     ( Reserved256 )
CPF_HAL_TAG     ( Reserved257 )
CPF_HAL_TAG     ( Reserved258 )
CPF_HAL_TAG     ( Reserved259 )
CPF_HAL_TAG     ( Reserved260 )
CPF_HAL_TAG     ( Reserved261 )
CPF_HAL_TAG     ( Reserved262 )
CPF_HAL_TAG     ( Reserved263 )
CPF_HAL_TAG     ( Reserved264 )
CPF_HAL_TAG     ( Reserved265 )
CPF_HAL_TAG     ( Reserved266 )
CPF_HAL_TAG     ( Reserved267 )
CPF_HAL_TAG     ( Reserved268 )
CPF_HAL_TAG     ( Reserved269 )
CPF_HAL_TAG     ( Reserved270 )
CPF_HAL_TAG     ( Reserved271 )
CPF_HAL_TAG     ( Reserved272 )
CPF_HAL_TAG     ( Reserved273 )
CPF_HAL_TAG     ( Reserved274 )
CPF_HAL_TAG     ( Reserved275 )
CPF_HAL_TAG     ( Reserved276 )
CPF_HAL_TAG     ( Reserved277 )
CPF_HAL_TAG     ( Reserved278 )
CPF_HAL_TAG     ( Reserved279 )
CPF_HAL_TAG     ( Reserved280 )
CPF_HAL_TAG     ( Reserved281 )
CPF_HAL_TAG     ( Reserved282 )
CPF_HAL_TAG     ( Reserved283 )
CPF_HAL_TAG     ( Reserved284 )
CPF_HAL_TAG     ( Reserved285 )
CPF_HAL_TAG     ( Reserved286 )
CPF_HAL_TAG     ( Reserved287 )
CPF_HAL_TAG     ( Reserved288 )
CPF_HAL_TAG     ( Reserved289 )
CPF_HAL_TAG     ( Reserved290 )
CPF_HAL_TAG     ( Reserved291 )
CPF_HAL_TAG     ( Reserved292 )
CPF_HAL_TAG     ( Reserved293 )
CPF_HAL_TAG     ( Reserved294 )
CPF_HAL_TAG     ( Reserved295 )
CPF_HAL_TAG     ( Reserved296 )
CPF_HAL_TAG     ( Reserved297 )
CPF_HAL_TAG     ( Reserved298 )
CPF_HAL_TAG     ( Reserved299 )
/*
** Finally, tags that do not fit into any of the above category.
** Most notably we have here custom qualities for property tables
** related to some particular tag, i.e. tags that specify qualities
** like "min", "max" or "default", but who are specific enough to
** some particular setting or feature so that they cannot be used
** in any other context.
*/
CPF_HAL_TAG     ( Reserved300 )

CPF_HAL_TAG_END

#ifdef __cplusplus
} // namespace CPF
#endif

