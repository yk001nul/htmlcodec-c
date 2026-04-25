#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "css-tokenizer.h"

/* =========================================================================
   Requirement 1 – CSS pattern codebook (11 segments)
   ========================================================================= */

const char* CSS_PATTERNS[] = {

    /* --------------------------------------------------------------------- */
    /* Segment 1 (CSS_SEG1_START = 0): HTML type selectors (tag names)        */
    /* --------------------------------------------------------------------- */
    /* void elements */
    "area", "base", "br", "col", "embed", "hr", "img", "input",
    "link", "meta", "param", "source", "track", "wbr",
    /* headings */
    "h1", "h2", "h3", "h4", "h5", "h6",
    /* root / document */
    "html", "head", "body",
    /* sectioning */
    "article", "aside", "footer", "header", "main", "nav", "section",
    /* content grouping */
    "address", "blockquote", "div", "dl", "dd", "dt", "figcaption",
    "figure", "li", "menu", "ol", "p", "pre", "ul",
    /* text-level semantics */
    "a", "abbr", "b", "bdi", "bdo", "cite", "code", "data", "dfn",
    "em", "i", "kbd", "mark", "q", "rp", "rt", "ruby", "s", "samp",
    "small", "span", "strong", "sub", "sup", "time", "u", "var",
    /* interactive */
    "button", "details", "dialog", "summary",
    /* forms */
    "datalist", "fieldset", "form", "label", "legend", "meter",
    "optgroup", "option", "output", "progress", "select", "textarea",
    /* media / embedded */
    "audio", "canvas", "iframe", "map", "object", "picture", "video",
    /* table */
    "caption", "colgroup", "table", "tbody", "td", "tfoot", "th",
    "thead", "tr",
    /* scripting / metadata */
    "noscript", "script", "slot", "style", "template", "title",
    /* edits */
    "del", "ins",
    /* SVG */
    "svg", "path", "circle", "rect", "polygon", "polyline", "line",
    "text", "tspan", "g", "defs", "use", "symbol", "clipPath", "mask",
    "filter", "feBlend", "feColorMatrix", "feComposite",
    "feGaussianBlur", "feOffset", "feTile", "feTurbulence",
    "radialGradient", "linearGradient", "stop",
    /* MathML */
    "math",

    /* --------------------------------------------------------------------- */
    /* Segment 2 (CSS_SEG2_START = 116): HTML attribute names                 */
    /* --------------------------------------------------------------------- */
    /* global */
    "id", "class", "style", "title", "lang", "dir", "hidden",
    "tabindex", "accesskey", "contenteditable", "draggable",
    "spellcheck", "translate", "role", "slot", "part", "is",
    "exportparts", "popover", "inert",
    /* link / navigation */
    "href", "target", "rel", "hreflang", "download", "ping",
    "referrerpolicy",
    /* media / resource */
    "src", "srcset", "sizes", "loading", "decoding", "fetchpriority",
    "crossorigin", "integrity", "usemap", "ismap",
    /* image / dimension */
    "alt", "width", "height",
    /* form */
    "action", "method", "enctype", "novalidate", "autocomplete",
    "autofocus", "for", "form", "name", "value", "type", "placeholder",
    "disabled", "readonly", "required", "checked", "selected",
    "multiple", "size", "maxlength", "minlength", "min", "max", "step",
    "pattern", "list", "rows", "cols", "wrap", "dirname",
    "popovertarget", "popovertargetaction",
    /* media element */
    "controls", "autoplay", "loop", "muted", "preload", "poster",
    /* track */
    "default", "kind", "srclang", "label",
    /* table */
    "colspan", "rowspan", "headers", "scope", "span", "abbr",
    /* iframe / embed */
    "allow", "allowfullscreen", "srcdoc", "sandbox",
    /* ordered list */
    "start", "reversed",
    /* details / blockquote / del / ins */
    "open", "cite", "datetime",
    /* scripting */
    "async", "defer", "charset",
    /* meta / link */
    "content", "http-equiv", "media",
    /* coordinates */
    "shape", "coords",
    /* ARIA */
    "aria-label", "aria-hidden", "aria-describedby", "aria-labelledby",
    "aria-expanded", "aria-controls", "aria-live", "aria-atomic",
    "aria-required", "aria-selected", "aria-checked", "aria-disabled",
    "aria-readonly", "aria-valuemin", "aria-valuemax", "aria-valuenow",
    "aria-orientation", "aria-relevant", "aria-busy",
    /* data-* prefix (the literal token) */
    "data",
    /* enterkeyhint / inputmode */
    "enterkeyhint", "inputmode",

    /* --------------------------------------------------------------------- */
    /* Segment 3 (CSS_SEG3_START = 233): Pseudo-class selectors               */
    /* (include the leading ':')                                               */
    /* --------------------------------------------------------------------- */
    ":hover", ":active", ":focus", ":visited", ":link", ":checked",
    ":disabled", ":enabled", ":first-child", ":last-child",
    ":only-child", ":first-of-type", ":last-of-type", ":only-of-type",
    ":root", ":empty", ":any-link", ":local-link", ":target",
    ":target-within", ":scope", ":focus-within", ":focus-visible",
    ":placeholder-shown", ":default", ":indeterminate", ":valid",
    ":invalid", ":in-range", ":out-of-range", ":required", ":optional",
    ":read-only", ":read-write", ":defined", ":modal", ":fullscreen",
    ":picture-in-picture", ":autofill", ":blank", ":popover-open",
    ":user-invalid", ":user-valid", ":playing", ":paused",
    ":past", ":current", ":future",
    /* functional pseudo-classes (include up to first '(') */
    ":nth-child(", ":nth-last-child(", ":nth-of-type(",
    ":nth-last-of-type(", ":not(", ":is(", ":where(", ":has(",
    ":lang(", ":dir(", ":nth-col(", ":nth-last-col(",

    /* --------------------------------------------------------------------- */
    /* Segment 4 (CSS_SEG4_START = 293): Pseudo-element selectors             */
    /* (include the leading '::')                                              */
    /* --------------------------------------------------------------------- */
    "::before", "::after", "::first-line", "::first-letter",
    "::selection", "::marker", "::placeholder", "::file-selector-button",
    "::backdrop", "::spelling-error", "::grammar-error", "::cue",
    "::cue-region", "::view-transition", "::highlight(",
    "::slotted(", "::part(",
    /* vendor */
    "::-webkit-scrollbar", "::-webkit-scrollbar-thumb",
    "::-webkit-scrollbar-track", "::-webkit-input-placeholder",
    "::-moz-placeholder",

    /* --------------------------------------------------------------------- */
    /* Segment 5 (CSS_SEG5_START = 316): CSS properties                       */
    /* --------------------------------------------------------------------- */
    /* display / position */
    "display", "position", "top", "right", "bottom", "left", "float",
    "clear", "overflow", "overflow-x", "overflow-y", "overflow-wrap",
    "text-overflow", "visibility", "z-index", "clip", "clip-path",
    "aspect-ratio",
    /* inset shorthand */
    "inset", "inset-block", "inset-inline", "inset-block-start",
    "inset-block-end", "inset-inline-start", "inset-inline-end",
    /* width / height */
    "width", "height", "min-width", "max-width", "min-height",
    "max-height", "box-sizing",
    /* margin */
    "margin", "margin-top", "margin-right", "margin-bottom",
    "margin-left", "margin-block", "margin-inline",
    "margin-block-start", "margin-block-end",
    "margin-inline-start", "margin-inline-end",
    /* padding */
    "padding", "padding-top", "padding-right", "padding-bottom",
    "padding-left", "padding-block", "padding-inline",
    "padding-block-start", "padding-block-end",
    "padding-inline-start", "padding-inline-end",
    /* flex */
    "flex", "flex-direction", "flex-wrap", "flex-flow", "flex-grow",
    "flex-shrink", "flex-basis", "justify-content", "justify-items",
    "justify-self", "align-items", "align-self", "align-content",
    "order", "gap", "row-gap", "column-gap", "place-items",
    "place-content", "place-self",
    /* grid */
    "grid", "grid-template", "grid-template-columns",
    "grid-template-rows", "grid-template-areas", "grid-auto-columns",
    "grid-auto-rows", "grid-auto-flow", "grid-area", "grid-column",
    "grid-column-start", "grid-column-end", "grid-row",
    "grid-row-start", "grid-row-end",
    /* font */
    "font", "font-family", "font-size", "font-weight", "font-style",
    "font-variant", "font-stretch", "font-display", "font-kerning",
    "font-feature-settings", "font-variation-settings",
    "font-language-override", "font-synthesis", "font-optical-sizing",
    "font-size-adjust", "font-palette",
    /* text */
    "line-height", "letter-spacing", "word-spacing",
    "text-align", "text-align-last", "text-decoration",
    "text-decoration-line", "text-decoration-style",
    "text-decoration-color", "text-decoration-thickness",
    "text-underline-offset", "text-underline-position",
    "text-transform", "text-indent", "text-shadow",
    "text-rendering", "text-emphasis", "text-emphasis-style",
    "text-emphasis-color", "text-emphasis-position",
    "vertical-align", "white-space", "word-break", "word-wrap",
    "hyphens", "hyphenate-character", "tab-size",
    "direction", "unicode-bidi", "writing-mode", "text-orientation",
    "text-combine-upright", "line-clamp", "-webkit-line-clamp",
    "-webkit-box-orient",
    /* color / background */
    "color", "background", "background-color", "background-image",
    "background-repeat", "background-position", "background-position-x",
    "background-position-y", "background-size", "background-attachment",
    "background-clip", "background-origin", "background-blend-mode",
    "opacity",
    /* border */
    "border", "border-top", "border-right", "border-bottom",
    "border-left", "border-width", "border-top-width",
    "border-right-width", "border-bottom-width", "border-left-width",
    "border-style", "border-top-style", "border-right-style",
    "border-bottom-style", "border-left-style", "border-color",
    "border-top-color", "border-right-color", "border-bottom-color",
    "border-left-color", "border-radius", "border-top-left-radius",
    "border-top-right-radius", "border-bottom-left-radius",
    "border-bottom-right-radius", "border-collapse", "border-spacing",
    "border-image", "border-image-source", "border-image-slice",
    "border-image-width", "border-image-outset", "border-image-repeat",
    "border-block", "border-block-start", "border-block-end",
    "border-inline", "border-inline-start", "border-inline-end",
    /* outline */
    "outline", "outline-width", "outline-style", "outline-color",
    "outline-offset",
    /* box-shadow */
    "box-shadow",
    /* list */
    "list-style", "list-style-type", "list-style-image",
    "list-style-position",
    /* table */
    "table-layout", "caption-side", "empty-cells",
    /* animation */
    "animation", "animation-name", "animation-duration",
    "animation-timing-function", "animation-delay",
    "animation-iteration-count", "animation-direction",
    "animation-fill-mode", "animation-play-state", "animation-timeline",
    /* transition */
    "transition", "transition-property", "transition-duration",
    "transition-timing-function", "transition-delay",
    /* transform */
    "transform", "transform-origin", "transform-style", "transform-box",
    "perspective", "perspective-origin", "backface-visibility",
    "rotate", "scale", "translate",
    /* appearance / interaction */
    "cursor", "pointer-events", "user-select", "resize", "appearance",
    "touch-action", "will-change", "caret-color", "accent-color",
    /* visual effects */
    "filter", "backdrop-filter", "mix-blend-mode", "isolation",
    /* object */
    "object-fit", "object-position",
    /* content generation */
    "content", "counter-increment", "counter-reset", "counter-set",
    "quotes",
    /* paging / break */
    "orphans", "widows", "page-break-before", "page-break-after",
    "page-break-inside", "break-before", "break-after", "break-inside",
    "print-color-adjust",
    /* columns */
    "column-count", "column-width", "columns", "column-rule",
    "column-rule-width", "column-rule-style", "column-rule-color",
    "column-span", "column-fill",
    /* scroll */
    "scroll-behavior", "scroll-snap-type", "scroll-snap-align",
    "scroll-snap-stop", "scroll-margin", "scroll-margin-top",
    "scroll-margin-right", "scroll-margin-bottom", "scroll-margin-left",
    "scroll-padding", "scroll-padding-top", "scroll-padding-right",
    "scroll-padding-bottom", "scroll-padding-left",
    "overscroll-behavior", "overscroll-behavior-x",
    "overscroll-behavior-y",
    /* color scheme */
    "color-scheme", "forced-color-adjust",
    /* image */
    "image-rendering", "image-orientation",
    /* shape */
    "shape-outside", "shape-margin", "shape-image-threshold",
    /* masking */
    "mask", "mask-image", "mask-repeat", "mask-position", "mask-size",
    "mask-composite", "mask-mode", "mask-origin", "mask-clip",
    /* containment */
    "contain", "contain-intrinsic-size", "contain-intrinsic-width",
    "contain-intrinsic-height", "content-visibility", "container",
    "container-name", "container-type",
    /* SVG presentation */
    "fill", "fill-opacity", "fill-rule", "stroke", "stroke-width",
    "stroke-linecap", "stroke-linejoin", "stroke-miterlimit",
    "stroke-opacity", "stroke-dasharray", "stroke-dashoffset",
    "paint-order", "marker", "marker-start", "marker-mid",
    "marker-end", "stop-color", "stop-opacity", "flood-color",
    "flood-opacity", "lighting-color", "vector-effect",

    /* --------------------------------------------------------------------- */
    /* Segment 6 (CSS_SEG6_START = 638): CSS at-rules                         */
    /* --------------------------------------------------------------------- */
    "@charset", "@color-profile", "@container", "@counter-style",
    "@document", "@font-face", "@font-feature-values",
    "@font-palette-values", "@import", "@keyframes", "@layer",
    "@media", "@namespace", "@page", "@property", "@supports",
    "@viewport", "@scope", "@starting-style",

    /* --------------------------------------------------------------------- */
    /* Segment 7 (CSS_SEG7_START = 657): Combinators and conditional symbols  */
    /* --------------------------------------------------------------------- */
    "||", "|=", "^=", "$=", "*=", "~=",
    "~", ">", "+",
    "!important",

    /* --------------------------------------------------------------------- */
    /* Segment 8 (CSS_SEG8_START = 667): Reserved keyword values              */
    /* --------------------------------------------------------------------- */
    /* global */
    "inherit", "initial", "unset", "revert", "revert-layer",
    /* display / layout */
    "auto", "none", "normal", "block", "inline", "flex", "grid",
    "table", "list-item", "inline-block", "inline-flex", "inline-grid",
    "inline-table", "contents", "flow", "flow-root",
    /* color */
    "currentColor", "transparent",
    /* font weight */
    "bold", "bolder", "lighter",
    /* font style */
    "italic", "oblique",
    /* text decoration */
    "underline", "overline", "line-through", "blink",
    /* border style */
    "solid", "dashed", "dotted", "double", "groove", "ridge",
    "inset", "outset", "hidden",
    /* visibility / overflow */
    "visible", "scroll", "clip",
    /* position */
    "static", "relative", "absolute", "fixed", "sticky",
    /* float / clear */
    "left", "right", "center", "top", "bottom", "both",
    /* animation / transition */
    "infinite", "alternate", "alternate-reverse", "forwards",
    "backwards", "paused", "running",
    "ease", "linear", "ease-in", "ease-out", "ease-in-out",
    "step-start", "step-end",
    /* flex / grid */
    "start", "end", "baseline", "stretch", "space-between",
    "space-around", "space-evenly",
    "wrap", "nowrap", "wrap-reverse",
    "row", "row-reverse", "column", "column-reverse",
    "dense", "auto-fill", "auto-fit",
    /* text */
    "uppercase", "lowercase", "capitalize", "justify",
    "pre", "pre-wrap", "pre-line", "break-word",
    "ltr", "rtl", "break-all", "keep-all",
    /* transform */
    "preserve-3d", "flat",
    /* object-fit */
    "fill", "contain", "cover", "scale-down",
    /* background */
    "no-repeat", "repeat-x", "repeat-y", "repeat", "round", "space",
    "local",
    /* border-image / background-origin */
    "border-box", "padding-box", "content-box",
    /* cursor */
    "default", "pointer", "crosshair", "move", "text", "wait",
    "help", "progress", "not-allowed", "no-drop", "grab", "grabbing",
    "zoom-in", "zoom-out", "copy", "alias", "cell",
    "vertical-text", "context-menu", "all-scroll",
    "n-resize", "s-resize", "e-resize", "w-resize",
    "ne-resize", "nw-resize", "se-resize", "sw-resize",
    "ew-resize", "ns-resize", "nesw-resize", "nwse-resize",
    "col-resize", "row-resize",
    "pan-x", "pan-y", "pan-left", "pan-right", "pan-up", "pan-down",
    "pinch-zoom", "manipulation",
    /* list-style-type */
    "disc", "circle", "square", "decimal", "decimal-leading-zero",
    "lower-roman", "upper-roman", "lower-alpha", "upper-alpha",
    "lower-greek", "lower-latin", "upper-latin",
    "armenian", "georgian",
    /* font family generic */
    "serif", "sans-serif", "monospace", "cursive", "fantasy",
    "system-ui", "ui-serif", "ui-sans-serif", "ui-monospace",
    "ui-rounded", "emoji", "math", "fangsong",
    /* image-rendering */
    "crisp-edges", "pixelated", "optimizeSpeed", "optimizeQuality",
    /* writing-mode */
    "horizontal-tb", "vertical-rl", "vertical-lr", "sideways-rl",
    "sideways-lr",
    /* user-select */
    "all",
    /* overflow-wrap */
    "anywhere",
    /* appearance */
    "none",
    /* misc keyword values */
    "odd", "even", "closest-side", "closest-corner",
    "farthest-side", "farthest-corner", "at",
    /* mix-blend-mode / isolation */
    "multiply", "screen", "overlay", "darken", "lighten",
    "color-dodge", "color-burn", "hard-light", "soft-light",
    "difference", "exclusion", "hue", "saturation",
    /* object-position / background-position */
    "center",

    /* --------------------------------------------------------------------- */
    /* Segment 9 (CSS_SEG9_START = 832): Value functions (up to first '(')   */
    /* --------------------------------------------------------------------- */
    "abs(", "acos(", "annotation(", "asin(", "atan(", "atan2(",
    "attr(", "blur(", "brightness(", "calc(", "character-variant(",
    "circle(", "clamp(", "color(", "color-mix(", "conic-gradient(",
    "contrast(", "cos(", "counter(", "counters(", "cross-fade(",
    "cubic-bezier(", "device-cmyk(", "drop-shadow(", "element(",
    "ellipse(", "env(", "exp(", "fit-content(", "format(",
    "grayscale(", "hsl(", "hsla(", "hue-rotate(", "hwb(", "hypot(",
    "image(", "image-set(", "inset(", "invert(", "lab(", "layer(",
    "lch(", "leader(", "linear(", "linear-gradient(", "local(",
    "log(", "matrix(", "matrix3d(", "max(", "min(", "minmax(",
    "mod(", "oklab(", "oklch(", "opacity(", "ornaments(", "paint(",
    "path(", "perspective(", "polygon(", "pow(", "radial-gradient(",
    "rem(", "repeat(", "repeating-conic-gradient(",
    "repeating-linear-gradient(", "repeating-radial-gradient(",
    "rgb(", "rgba(", "rotate(", "rotate3d(", "rotateX(", "rotateY(",
    "rotateZ(", "round(", "saturate(", "scale(", "scale3d(",
    "scaleX(", "scaleY(", "scaleZ(", "sepia(", "sign(", "sin(",
    "skew(", "skewX(", "skewY(", "sqrt(", "steps(", "styleset(",
    "stylistic(", "supports(", "swash(", "symbols(", "tan(",
    "target-counter(", "target-counters(", "translate(",
    "translate3d(", "translateX(", "translateY(", "translateZ(",
    "url(", "var(",

    /* --------------------------------------------------------------------- */
    /* Segment 10 (CSS_SEG10_START = 937): Named colors                       */
    /* --------------------------------------------------------------------- */
    "aliceblue", "antiquewhite", "aqua", "aquamarine", "azure",
    "beige", "bisque", "black", "blanchedalmond", "blue",
    "blueviolet", "brown", "burlywood", "cadetblue", "chartreuse",
    "chocolate", "coral", "cornflowerblue", "cornsilk", "crimson",
    "cyan", "darkblue", "darkcyan", "darkgoldenrod", "darkgray",
    "darkgreen", "darkgrey", "darkkhaki", "darkmagenta",
    "darkolivegreen", "darkorange", "darkorchid", "darkred",
    "darksalmon", "darkseagreen", "darkslateblue", "darkslategray",
    "darkslategrey", "darkturquoise", "darkviolet", "deeppink",
    "deepskyblue", "dimgray", "dimgrey", "dodgerblue", "firebrick",
    "floralwhite", "forestgreen", "fuchsia", "gainsboro",
    "ghostwhite", "gold", "goldenrod", "gray", "green",
    "greenyellow", "grey", "honeydew", "hotpink", "indianred",
    "indigo", "ivory", "khaki", "lavender", "lavenderblush",
    "lawngreen", "lemonchiffon", "lightblue", "lightcoral",
    "lightcyan", "lightgoldenrodyellow", "lightgray", "lightgreen",
    "lightgrey", "lightpink", "lightsalmon", "lightseagreen",
    "lightskyblue", "lightslategray", "lightslategrey",
    "lightsteelblue", "lightyellow", "lime", "limegreen", "linen",
    "magenta", "maroon", "mediumaquamarine", "mediumblue",
    "mediumorchid", "mediumpurple", "mediumseagreen",
    "mediumslateblue", "mediumspringgreen", "mediumturquoise",
    "mediumvioletred", "midnightblue", "mintcream", "mistyrose",
    "moccasin", "navajowhite", "navy", "oldlace", "olive",
    "olivedrab", "orange", "orangered", "orchid", "palegoldenrod",
    "palegreen", "paleturquoise", "palevioletred", "papayawhip",
    "peachpuff", "peru", "pink", "plum", "powderblue", "purple",
    "rebeccapurple", "red", "rosybrown", "royalblue", "saddlebrown",
    "salmon", "sandybrown", "seagreen", "seashell", "sienna",
    "silver", "skyblue", "slateblue", "slategray", "slategrey",
    "snow", "springgreen", "steelblue", "tan", "teal", "thistle",
    "tomato", "turquoise", "violet", "wheat", "white", "whitesmoke",
    "yellow", "yellowgreen",

    /* --------------------------------------------------------------------- */
    /* Segment 11 (CSS_SEG11_START = 1085): Named blocks in CSS at-rules      */
    /* --------------------------------------------------------------------- */
    /* @keyframes control points */
    "from", "to",
    /* @media media types */
    "screen", "print", "speech",
    /* @page named pages and margin boxes */
    "landscape", "portrait", "first",
    "top-left-corner", "top-left", "top-center", "top-right",
    "top-right-corner",
    "bottom-left-corner", "bottom-left", "bottom-center",
    "bottom-right", "bottom-right-corner",
    "left-top", "left-middle", "left-bottom",
    "right-top", "right-middle", "right-bottom",
    /* @font-face format hints */
    "woff2", "woff", "truetype", "opentype", "embedded-opentype",
    "svg",
};

/* Segment start indices ---------------------------------------------------- */
/* Seg1  tags:         0   .. 137  (138 entries)                             */
/* Seg2  attributes:   138 .. 264  (127 entries)                             */
/* Seg3  pseudo-class: 265 .. 324  ( 60 entries)                             */
/* Seg4  pseudo-elem:  325 .. 346  ( 22 entries)                             */
/* Seg5  properties:   347 .. 678  (332 entries)                             */
/* Seg6  at-rules:     679 .. 697  ( 19 entries)                             */
/* Seg7  combinators:  698 .. 707  ( 10 entries)                             */
/* Seg8  keywords:     708 .. 923  (216 entries)                             */
/* Seg9  functions:    924 .. 1029 (106 entries)                             */
/* Seg10 colors:       1030.. 1177 (148 entries)                             */
/* Seg11 named blocks: 1178.. 1207 ( 30 entries)  total: 1208               */
const int CSS_SEG1_START  = 0;
const int CSS_SEG2_START  = 138;
const int CSS_SEG3_START  = 265;
const int CSS_SEG4_START  = 325;
const int CSS_SEG5_START  = 347;
const int CSS_SEG6_START  = 679;
const int CSS_SEG7_START  = 698;
const int CSS_SEG8_START  = 708;
const int CSS_SEG9_START  = 924;
const int CSS_SEG10_START = 1030;
const int CSS_SEG11_START = 1178;

const int CSS_PATTERN_COUNT =
    (int)(sizeof(CSS_PATTERNS) / sizeof(CSS_PATTERNS[0]));


/* =========================================================================
   Pattern matching helpers
   ========================================================================= */

/* Returns the index of `str` in CSS_PATTERNS (exact match), or -1. */
static int css_find_pattern(const char* str) {
    for (int i = 0; i < CSS_PATTERN_COUNT; i++) {
        if (strcmp(str, CSS_PATTERNS[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/* Tokenize `str` using whole-string matching (Requirements 3 & 4).
   If `str` exactly matches a pattern:  one CSSTokenizable (isPattern=true).
   Otherwise: one CSSTokenizable per character (isPattern=false, flag=ASCII). */
static void css_tokenize_exact(const char* str,
                               CSSTokenizable* tokens,
                               int* size) {
    *size = 0;
    int idx = css_find_pattern(str);
    if (idx >= 0) {
        tokens[0].isPattern = true;
        tokens[0].flag      = (unsigned short)idx;
        *size = 1;
    } else {
        int len = (int)strlen(str);
        for (int i = 0; i < len && *size < CSS_MAX_TOKENIZABLE; i++) {
            tokens[*size].isPattern = false;
            tokens[*size].flag      = (unsigned short)(unsigned char)str[i];
            (*size)++;
        }
    }
}

/* Tokenize `str` using greedy longest-match scanning (Requirement 5).
   At each position attempt every pattern; take the longest match.
   If no pattern matches, emit one ASCII CSSTokenizable and advance by 1. */
static void css_tokenize_atrule(const char* str,
                                CSSTokenizable* tokens,
                                int* size) {
    *size = 0;
    int pos = 0;
    int len = (int)strlen(str);

    while (pos < len && *size < CSS_MAX_TOKENIZABLE) {
        int remaining = len - pos;
        int best_idx  = -1;
        int best_len  = 0;

        for (int i = 0; i < CSS_PATTERN_COUNT; i++) {
            int plen = (int)strlen(CSS_PATTERNS[i]);
            /* Guardrail: skip patterns longer than the remaining input */
            if (plen > remaining || plen <= best_len) continue;
            if (strncmp(str + pos, CSS_PATTERNS[i], (size_t)plen) == 0) {
                best_idx = i;
                best_len = plen;
            }
        }

        if (best_idx >= 0) {
            tokens[*size].isPattern = true;
            tokens[*size].flag      = (unsigned short)best_idx;
            (*size)++;
            pos += best_len;
        } else {
            tokens[*size].isPattern = false;
            tokens[*size].flag      = (unsigned short)(unsigned char)str[pos];
            (*size)++;
            pos++;
        }
    }
}


/* =========================================================================
   Public API
   ========================================================================= */

void parseCSS(const char* css, CSSTokenArray* result) {
    result->count = 0;
    int i = 0, len = (int)strlen(css);

    while (i < len && result->count < CSS_MAX_TOKENS) {
        /* Skip whitespace */
        while (i < len && isspace((unsigned char)css[i])) i++;
        if (i >= len) break;

        /* --- Comments ----------------------------------------------- */
        if (i + 1 < len && css[i] == '/' && css[i + 1] == '*') {
            int commentStart = i;
            i += 2;
            while (i + 1 < len && !(css[i] == '*' && css[i + 1] == '/')) i++;
            if (i + 1 < len) i += 2;

            int commentLen = i - commentStart;
            CSSToken* token = &result->tokens[result->count++];
            token->type = 2;
            int copyLen = commentLen < CSS_MAX_PROPERTY_VALUE
                          ? commentLen : CSS_MAX_PROPERTY_VALUE - 1;
            strncpy(token->data.comment.content, &css[commentStart], (size_t)copyLen);
            token->data.comment.content[copyLen] = '\0';

            /* Req 1 (csscodec): character-level tokenization of comment */
            token->data.comment.commentTokenSize = 0;
            for (int ci = 0; ci < copyLen &&
                 token->data.comment.commentTokenSize < CSS_MAX_TOKENIZABLE; ci++) {
                token->data.comment.commentTokens[token->data.comment.commentTokenSize].isPattern = false;
                token->data.comment.commentTokens[token->data.comment.commentTokenSize].flag =
                    (unsigned short)(unsigned char)token->data.comment.content[ci];
                token->data.comment.commentTokenSize++;
            }
            continue;
        }

        /* --- At-rules ----------------------------------------------- */
        if (css[i] == '@') {
            int ruleStart = i;
            while (i < len && css[i] != '{' && css[i] != ';') i++;

            int ruleLen = i - ruleStart;
            /* Trim trailing whitespace */
            while (ruleLen > 0 &&
                   isspace((unsigned char)css[ruleStart + ruleLen - 1]))
                ruleLen--;

            CSSToken* token = &result->tokens[result->count++];
            token->type = 1;
            int copyLen = ruleLen < CSS_MAX_SELECTOR_LEN
                          ? ruleLen : CSS_MAX_SELECTOR_LEN - 1;
            strncpy(token->data.atRule.rule, &css[ruleStart], (size_t)copyLen);
            token->data.atRule.rule[copyLen] = '\0';

            /* Requirement 5: tokenize the at-rule string */
            css_tokenize_atrule(token->data.atRule.rule,
                                token->data.atRule.atRuleTokens,
                                &token->data.atRule.atRuleTokenSize);

            /* Consume block or statement */
            if (i < len && css[i] == '{') {
                int braceCount = 1;
                i++;
                while (i < len && braceCount > 0) {
                    if (css[i] == '{') braceCount++;
                    else if (css[i] == '}') braceCount--;
                    i++;
                }
            } else if (i < len && css[i] == ';') {
                i++;
            }
            continue;
        }

        /* --- Selector rules ----------------------------------------- */
        int selectorStart = i;
        while (i < len && css[i] != '{') i++;
        int selectorLen = i - selectorStart;

        /* Trim trailing whitespace */
        while (selectorLen > 0 &&
               isspace((unsigned char)css[selectorStart + selectorLen - 1]))
            selectorLen--;

        if (i >= len || selectorLen == 0) break;

        CSSToken* token = &result->tokens[result->count++];
        token->type = 0;
        int copyLen = selectorLen < CSS_MAX_SELECTOR_LEN
                      ? selectorLen : CSS_MAX_SELECTOR_LEN - 1;
        strncpy(token->data.rule.selector, &css[selectorStart], (size_t)copyLen);
        token->data.rule.selector[copyLen] = '\0';

        /* Requirement 4: tokenize the selector (greedy longest-match, same as at-rules) */
        css_tokenize_atrule(token->data.rule.selector,
                            token->data.rule.selectorTokens,
                            &token->data.rule.selectorTokenSize);

        i++; /* skip '{' */

        /* --- Properties --------------------------------------------- */
        token->data.rule.propertyCount = 0;
        while (i < len && css[i] != '}' &&
               token->data.rule.propertyCount < CSS_MAX_PROPERTIES) {
            while (i < len && isspace((unsigned char)css[i])) i++;
            if (i >= len || css[i] == '}') break;

            /* Property name */
            int propNameStart = i;
            while (i < len && css[i] != ':' && css[i] != '}') i++;
            int propNameLen = i - propNameStart;
            while (propNameLen > 0 &&
                   isspace((unsigned char)css[propNameStart + propNameLen - 1]))
                propNameLen--;

            if (propNameLen == 0 || i >= len || css[i] != ':') break;

            CSSProperty* prop =
                &token->data.rule.properties[token->data.rule.propertyCount];

            int pnCopy = propNameLen < CSS_MAX_PROPERTY_NAME
                         ? propNameLen : CSS_MAX_PROPERTY_NAME - 1;
            strncpy(prop->name, &css[propNameStart], (size_t)pnCopy);
            prop->name[pnCopy] = '\0';

            i++; /* skip ':' */

            /* Property value */
            while (i < len && isspace((unsigned char)css[i])) i++;
            int propValueStart = i;
            while (i < len && css[i] != ';' && css[i] != '}') i++;
            int propValueLen = i - propValueStart;
            while (propValueLen > 0 &&
                   isspace((unsigned char)css[propValueStart + propValueLen - 1]))
                propValueLen--;

            int pvCopy = propValueLen < CSS_MAX_PROPERTY_VALUE
                         ? propValueLen : CSS_MAX_PROPERTY_VALUE - 1;
            strncpy(prop->value, &css[propValueStart], (size_t)pvCopy);
            prop->value[pvCopy] = '\0';

            /* Requirement 3: tokenize name and value */
            css_tokenize_exact(prop->name,  prop->nameTokens,
                               &prop->nameTokenSize);
            css_tokenize_exact(prop->value, prop->valueTokens,
                               &prop->valueTokenSize);

            token->data.rule.propertyCount++;

            if (i < len && css[i] == ';') i++;
        }

        /* Req 2 (csscodec): flatten selector + properties into ruleTokens */
        css_flatten_rule_tokens(token);

        if (i < len && css[i] == '}') i++;
    }
}

void freeCSS(CSSTokenArray* arr) {
    if (arr != NULL) {
        free(arr);
    }
}

/* =========================================================================
   Req 2 (csscodec) – css_flatten_rule_tokens
   ========================================================================= */

void css_flatten_rule_tokens(CSSToken* token) {
    if (!token || token->type != 0) return;

    CSSTokenizable* out = token->data.rule.ruleTokens;
    int size = 0;

#define APPEND(ip, fl) \
    do { if (size < CSS_MAX_TOKENIZABLE) { \
        out[size].isPattern = (ip); \
        out[size].flag = (unsigned short)(fl); \
        size++; \
    } } while (0)

    /* selector tokens */
    for (int i = 0; i < token->data.rule.selectorTokenSize &&
                    size < CSS_MAX_TOKENIZABLE; i++) {
        out[size++] = token->data.rule.selectorTokens[i];
    }

    APPEND(false, '{');

    for (int p = 0; p < token->data.rule.propertyCount; p++) {
        const CSSProperty* prop = &token->data.rule.properties[p];

        for (int i = 0; i < prop->nameTokenSize && size < CSS_MAX_TOKENIZABLE; i++)
            out[size++] = prop->nameTokens[i];

        APPEND(false, ':');

        for (int i = 0; i < prop->valueTokenSize && size < CSS_MAX_TOKENIZABLE; i++)
            out[size++] = prop->valueTokens[i];

        APPEND(false, ';');
    }

    APPEND(false, '}');

#undef APPEND

    token->data.rule.ruleTokenSize = size;
}

/* =========================================================================
   Req 3 (csscodec) – collectCSSFrequencies / freeCSSFreqMap
   ========================================================================= */

static int css_freq_cmp(const void* a, const void* b) {
    return ((const CSSFreqEntry*)b)->frequency -
           ((const CSSFreqEntry*)a)->frequency;
}

CSSFreqMap* collectCSSFrequencies(const CSSTokenArray* arr) {
    if (!arr) return NULL;

    CSSFreqMap* map = (CSSFreqMap*)calloc(1, sizeof(CSSFreqMap));
    if (!map) return NULL;

    for (int t = 0; t < arr->count; t++) {
        const CSSToken* tok = &arr->tokens[t];
        const CSSTokenizable* src = NULL;
        int srcSize = 0;

        if (tok->type == 0) {
            src     = tok->data.rule.ruleTokens;
            srcSize = tok->data.rule.ruleTokenSize;
        } else if (tok->type == 1) {
            src     = tok->data.atRule.atRuleTokens;
            srcSize = tok->data.atRule.atRuleTokenSize;
        } else {
            src     = tok->data.comment.commentTokens;
            srcSize = tok->data.comment.commentTokenSize;
        }

        for (int i = 0; i < srcSize; i++) {
            map->totalTokens++;
            /* find existing entry */
            int found = -1;
            for (int e = 0; e < map->uniqueCount; e++) {
                if (map->entries[e].token.isPattern == src[i].isPattern &&
                    map->entries[e].token.flag      == src[i].flag) {
                    found = e;
                    break;
                }
            }
            if (found >= 0) {
                map->entries[found].frequency++;
            } else if (map->uniqueCount < CSS_MAX_UNIQUE_TOKENIZABLE) {
                map->entries[map->uniqueCount].token = src[i];
                map->entries[map->uniqueCount].frequency = 1;
                map->uniqueCount++;
            }
        }
    }

    qsort(map->entries, (size_t)map->uniqueCount,
          sizeof(CSSFreqEntry), css_freq_cmp);

    return map;
}

void freeCSSFreqMap(CSSFreqMap* map) {
    free(map);
}
