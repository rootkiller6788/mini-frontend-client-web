# API Reference — mini-frontend-engineering

## Modules

### 1. bundler_core.h — Module Bundler

#### Types

| Type | Description |
|------|-------------|
| `ModuleType` | `MODULE_ESM`, `MODULE_CJS`, `MODULE_UNKNOWN` |
| `ChunkType` | `CHUNK_ENTRY`, `CHUNK_VENDOR`, `CHUNK_APP`, `CHUNK_DYNAMIC` |
| `Module` | Represents a single source module with dependencies & exports |
| `Chunk` | A group of modules bundled together |
| `BundlerConfig` | Top-level configuration for bundling |
| `ResolveConfig` | Module resolution settings (extensions, aliases) |
| `Bundler` | The main bundler instance |

#### Functions

```
void bundler_init(Bundler *b, const char *root);
```
Initialize the bundler with a project root directory.

```
int bundler_add_entry(Bundler *b, const char *entry_path);
```
Add an entry point file. Returns 0 on success, -1 on failure.

```
int bundler_resolve_module(Bundler *b, const char *base_path, const char *specifier, char *out_path, size_t out_len);
```
Resolve a module specifier (like `./math` or `lodash`) to an absolute file path.
Supports relative imports, extension resolution, and `node_modules`.

```
int bundler_parse_module(Bundler *b, const char *module_path, Module *mod);
```
Parse a module file: detect type (ESM/CJS), extract imports, exports, and require() calls.

```
int bundler_build_dependency_graph(Bundler *b);
```
Recursively traverse entry points and build the full dependency graph.

```
void bundler_split_chunks(Bundler *b);
```
Split modules into vendor chunk (node_modules) and app chunk (project source).

```
int bundler_generate_bundle(Bundler *b);
```
Generate bundled output with IIFE wrappers for each chunk.

```
void bundler_write_output(Bundler *b, const char *output_dir);
```
Write all chunks to disk.

```
void bundler_print_stats(Bundler *b);
```
Print bundle statistics (module count, chunk sizes, total size).

---

### 2. tree_shaker.h — Tree Shaking

#### Types

| Type | Description |
|------|-------------|
| `ExportKind` | `EXPORT_NAMED`, `EXPORT_DEFAULT`, `EXPORT_ALL`, `EXPORT_NAMESPACE` |
| `ImportKind` | `IMPORT_NAMED`, `IMPORT_DEFAULT`, `IMPORT_NAMESPACE`, `IMPORT_DYNAMIC` |
| `ASTNodeKind` | AST node types for static analysis |
| `ASTNode` | Simplified AST node for source code |
| `ShakerExport` | Tracked export with usage status |
| `ShakerImport` | Tracked import with bound names |
| `ShakerModule` | A module being analyzed for tree shaking |
| `SideEffectEntry` | Side-effect declaration from package.json |
| `TreeShaker` | Main tree-shaking engine |

#### Functions

```
void shaker_init(TreeShaker *ts);
int  shaker_add_module(TreeShaker *ts, const char *path, const char *source, size_t len);
void shaker_set_entry(TreeShaker *ts, const char *module_path);
```
Set up the tree shaker and populate it with modules.

```
ASTNode* shaker_parse_source(const char *source, size_t len);
```
Parse source code into a simple AST for static analysis.

```
void shaker_analyze_exports(TreeShaker *ts, ShakerModule *mod);
void shaker_mark_reachable(TreeShaker *ts);
void shaker_mark_used_exports(TreeShaker *ts);
```
Three-pass analysis: identify exports, mark reachable modules from entry, mark actually used exports.

```
bool shaker_is_side_effect_free(TreeShaker *ts, const char *module_path);
```
Check if a module is declared side-effect-free (via package.json `"sideEffects": false`).

```
void shaker_eliminate_dead_code(TreeShaker *ts);
```
Remove unreachable code. Side-effect-free modules that are unused get fully removed.

```
void shaker_scope_hoisting(TreeShaker *ts);
```
Concatenate all reachable modules into a single scope to reduce function call overhead.

```
int  shaker_parse_package_json(TreeShaker *ts, const char *pkg_path);
void shaker_print_stats(TreeShaker *ts);
void shaker_free_ast(ASTNode *node);
```

---

### 3. hmr_engine.h — Hot Module Replacement

#### Types

| Type | Description |
|------|-------------|
| `HMRState` | State machine: `IDLE`, `CHECKING`, `READY`, `DISPOSING`, `APPLYING`, `FAILED` |
| `HMRModule` | A module tracked for HMR with hash and self-accept flag |
| `HMRUpdate` | A pending update with new source and affected modules |
| `HMRBoundary` | The HMR boundary module that accepts the update |
| `HMRClientConfig` | WebSocket URL, reconnect settings |
| `HMREngine` | Main HMR engine |

#### Functions

```
void hmr_init(HMREngine *e, const char *ws_url);
void hmr_connect(HMREngine *e);
void hmr_disconnect(HMREngine *e);
```
Lifecycle management.

```
int  hmr_register_module(HMREngine *e, const char *id, uint64_t hash);
void hmr_add_dependency(HMREngine *e, const char *parent_id, const char *child_id);
void hmr_set_self_accept(HMREngine *e, const char *module_id, bool accept);
```
Module registration and dependency tracking. `hmr_set_self_accept` marks a module as an HMR boundary.

```
int  hmr_handle_update(HMREngine *e, const char *json_payload);
```
Process an incoming JSON update from the dev server WebSocket.

```
void hmr_apply_updates(HMREngine *e);
```
Apply all pending updates. Walks dependency tree to find HMR boundaries. Falls back to full reload if no boundary found.

```
void hmr_save_state_snapshot(HMREngine *e);
void hmr_restore_state(HMREngine *e, int snapshot_index);
```
State preservation across hot updates (keeps last 4 snapshots).

```
void hmr_trigger_full_reload(HMREngine *e);
void hmr_send_status(HMREngine *e);
const char* hmr_state_string(HMRState state);
```

---

### 4. ssr_model.h — SSR / SSG / ISR

#### Types

| Type | Description |
|------|-------------|
| `SSRRenderMode` | `SSR_RENDER_STATIC`, `SSR_RENDER_STREAMING`, `SSR_RENDER_INCREMENTAL` |
| `HydrationState` | States for client-side hydration |
| `SSRProp` | A component property (name/value) |
| `HydrationNode` | A DOM node that needs event handler attachment |
| `SSRComponent` | A server-side component with props, HTML output, CSS |
| `SSRRouterEntry` | A route entry with optional ISR revalidation |
| `SSRDocument` | The HTML document template (head, body, scripts) |
| `SSRConfig` | Full SSR configuration |
| `SSREngine` | Main SSR engine with rendering stats |

#### Functions

```
void ssr_init(SSREngine *e);
void ssr_set_mode(SSREngine *e, SSRRenderMode mode);
```
Initialize engine and set rendering mode.

```
int  ssr_register_component(SSREngine *e, const char *name);
void ssr_set_prop(SSRComponent *comp, const char *name, const char *value);
void ssr_set_slot(SSRComponent *comp, int index, const char *content);
```
Define components and their data.

```
int  ssr_register_route(SSREngine *e, const char *path, const char *component_name);
void ssr_set_revalidate(SSRRouterEntry *route, int seconds);
```
Register routes (supports `:param` dynamic segments) and set ISR intervals.

```
void ssr_render_to_string(SSREngine *e, SSRComponent *comp, char *out, size_t *len);
void ssr_render_page(SSREngine *e, const char *path, char *out, size_t *len);
```
Render a component or full page to an HTML string.

```
void ssr_streaming_render(SSREngine *e, const char *path);
char* ssr_streaming_next_chunk(SSREngine *e);
bool ssr_streaming_has_more(SSREngine *e);
```
Streaming SSR: render in chunks for faster TTFB.

```
void ssr_hydrate(SSREngine *e, const char *html, size_t len);
void ssr_hydrate_node(HydrationNode *node);
void ssr_attach_event_handler(HydrationNode *node, const char *event, const char *handler);
```
Client-side hydration: scan server HTML and attach event listeners.

```
int  ssr_static_generate(SSREngine *e, const char *output_dir);
```
SSG: generate static HTML files for all routes at build time.

```
int  ssr_isr_check(SSREngine *e, const char *path);
void ssr_isr_regenerate(SSREngine *e, SSRRouterEntry *route);
```
ISR: check if a cached page needs revalidation, regenerate if stale.

```
void ssr_inject_scripts(SSREngine *e, char *html, size_t *len);
void ssr_inject_state(SSREngine *e, char *html, size_t *len);
void ssr_print_stats(SSREngine *e);
```

---

### 5. asset_pipeline.h — Asset Pipeline

#### Types

| Type | Description |
|------|-------------|
| `APImageFormat` | `PNG`, `JPEG`, `WEBP`, `AVIF`, `SVG`, `GIF` |
| `APResizeMode` | `CONTAIN`, `COVER`, `FILL`, `NONE` |
| `APQueueStatus` | `PENDING`, `PROCESSING`, `DONE`, `FAILED` |
| `APImageConfig` | Image optimization settings |
| `APSassVariable` | SCSS variable (name, value, scope) |
| `APSassContext` | SCSS compilation state |
| `APPrefixRule` | Autoprefixer rule for a CSS property |
| `APFile` | A file in the pipeline with source, processed output, hash |
| `AssetPipeline` | Main pipeline configuration |

#### Functions

```
void ap_init(AssetPipeline *ap, const char *input_dir, const char *output_dir);
int  ap_add_file(AssetPipeline *ap, const char *path);
```
Initialize with input/output directories and add files.

```
void ap_set_image_config(AssetPipeline *ap, const char *path, APImageFormat fmt, int w, int h, int quality);
void ap_image_resize(const char *in_path, const char *out_path, int w, int h, APResizeMode mode);
void ap_image_convert_format(const char *in_path, const char *out_path, APImageFormat fmt, int quality);
void ap_generate_srcset(AssetPipeline *ap, APImageConfig *cfg);
```
Image pipeline operations.

```
int  ap_compile_sass(AssetPipeline *ap, const char *scss_str, size_t len, char *out_css, size_t *out_len);
void ap_sass_define_variable(APSassContext *ctx, const char *name, const char *value, bool global);
void ap_sass_resolve_variables(APSassContext *ctx, char *scss, size_t *len);
void ap_sass_resolve_nesting(APSassContext *ctx, char *scss, size_t *len);
```
SCSS/Sass compilation: variables and nesting.

```
int  ap_autoprefix(AssetPipeline *ap, const char *css_str, size_t len, char *out_css, size_t *out_len);
void ap_register_prefix(AssetPipeline *ap, const char *property, bool webkit, bool moz, bool ms, bool o);
```
CSS Autoprefixer: add vendor prefixes for browser compatibility.

```
void ap_generate_sourcemap(AssetPipeline *ap, const char *original, size_t orig_len, const char *processed, size_t proc_len, char *out_map, size_t *map_len);
void ap_sourcemap_base64_inline(AssetPipeline *ap, APFile *file);
```
Source map generation and base64 inline embedding.

```
void ap_compute_content_hash(const char *data, size_t len, char out_hash[65]);
void ap_rename_with_hash(AssetPipeline *ap, APFile *file);
```
Content hashing for cache busting.

```
void ap_process_all(AssetPipeline *ap);
void ap_process_file(AssetPipeline *ap, int index);
void ap_print_report(AssetPipeline *ap);
```
Process files and print a summary report.
