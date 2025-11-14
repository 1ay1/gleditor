/* Simple Shader Library Test Program
 * Demonstrates basic usage of the NeoWall shader compilation library
 *
 * Compile: gcc simple_shader_test.c -I../src -L../bin -lGLESv2 -lm -o shader_test
 * Run: ./shader_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../src/shader_lib/neowall_shader_api.h"
#include "../src/shader_lib/shader_utils.h"

/* Test shader sources */
static const char *test_shadertoy =
    "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
    "    vec2 uv = fragCoord / iResolution.xy;\n"
    "    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));\n"
    "    fragColor = vec4(col, 1.0);\n"
    "}\n";

static const char *test_raw_fragment =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform float _neowall_time;\n"
    "uniform vec2 _neowall_resolution;\n"
    "void main() {\n"
    "    vec2 uv = gl_FragCoord.xy / _neowall_resolution;\n"
    "    gl_FragColor = vec4(uv, 0.5, 1.0);\n"
    "}\n";

static const char *test_invalid =
    "This is not valid GLSL code!";

/* Test results structure */
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} test_results_t;

static test_results_t results = {0, 0, 0};

/* Helper macros */
#define TEST_START(name) \
    printf("\n=== Test: %s ===\n", name); \
    results.tests_run++;

#define TEST_PASS() \
    printf("✓ PASSED\n"); \
    results.tests_passed++;

#define TEST_FAIL(msg) \
    printf("✗ FAILED: %s\n", msg); \
    results.tests_failed++;

#define ASSERT(condition, msg) \
    if (!(condition)) { \
        TEST_FAIL(msg); \
        return false; \
    }

/* Test functions */
bool test_compile_shadertoy(void) {
    TEST_START("Compile Shadertoy Format Shader");

    neowall_shader_result_t result = neowall_shader_compile(test_shadertoy, NULL);

    ASSERT(result.success, "Compilation should succeed");
    ASSERT(result.program != 0, "Program ID should be non-zero");
    ASSERT(result.error_message == NULL, "Should have no error message");

    neowall_shader_destroy(result.program);

    TEST_PASS();
    return true;
}

bool test_compile_raw_fragment(void) {
    TEST_START("Compile Raw Fragment Shader");

    neowall_shader_result_t result = neowall_shader_compile(test_raw_fragment, NULL);

    ASSERT(result.success, "Compilation should succeed");
    ASSERT(result.program != 0, "Program ID should be non-zero");

    neowall_shader_destroy(result.program);

    TEST_PASS();
    return true;
}

bool test_compile_invalid(void) {
    TEST_START("Compile Invalid Shader (Should Fail)");

    neowall_shader_result_t result = neowall_shader_compile(test_invalid, NULL);

    ASSERT(!result.success, "Compilation should fail");
    ASSERT(result.program == 0, "Program ID should be zero");
    ASSERT(result.error_message != NULL, "Should have error message");

    printf("Expected error message: %s\n", result.error_message);
    neowall_shader_free_result(&result);

    TEST_PASS();
    return true;
}

bool test_custom_options(void) {
    TEST_START("Compile with Custom Options");

    neowall_shader_options_t options = {
        .use_es3 = false,
        .channel_count = 2,
        .verbose_errors = true
    };

    neowall_shader_result_t result = neowall_shader_compile(test_shadertoy, &options);

    ASSERT(result.success, "Compilation should succeed");
    neowall_shader_destroy(result.program);

    TEST_PASS();
    return true;
}

bool test_shader_validation(void) {
    TEST_START("Shader Validation");

    shader_validation_t *val = shader_validate_syntax(test_shadertoy, true);

    ASSERT(val != NULL, "Validation result should not be NULL");
    ASSERT(val->is_valid, "Valid shader should pass validation");
    ASSERT(val->has_main, "Should detect main function");

    printf("Has version directive: %s\n", val->has_version ? "Yes" : "No");
    printf("Detected version: %d\n", val->detected_version);
    printf("Warnings: %zu\n", val->warning_count);
    printf("Errors: %zu\n", val->error_count);

    shader_free_validation(val);

    TEST_PASS();
    return true;
}

bool test_shader_statistics(void) {
    TEST_START("Shader Statistics");

    shader_stats_t *stats = shader_get_statistics(test_shadertoy);

    ASSERT(stats != NULL, "Statistics should not be NULL");

    printf("Lines: %zu\n", stats->line_count);
    printf("Uniforms: %zu\n", stats->uniform_count);
    printf("Textures: %zu\n", stats->texture_count);
    printf("Functions: %zu\n", stats->function_count);
    printf("Uses loops: %s\n", stats->uses_loops ? "Yes" : "No");
    printf("Uses conditionals: %s\n", stats->uses_conditionals ? "Yes" : "No");
    printf("Is Shadertoy format: %s\n", stats->is_shadertoy_format ? "Yes" : "No");
    printf("Complexity score: %d%%\n", stats->complexity_score);

    ASSERT(stats->is_shadertoy_format, "Should detect Shadertoy format");

    shader_free_stats(stats);

    TEST_PASS();
    return true;
}

bool test_shader_templates(void) {
    TEST_START("Shader Templates");

    size_t count;
    const char **templates = shader_list_templates(&count);

    ASSERT(templates != NULL, "Templates list should not be NULL");
    ASSERT(count > 0, "Should have at least one template");

    printf("Available templates:\n");
    for (size_t i = 0; i < count; i++) {
        printf("  - %s\n", templates[i]);
        const char *template = shader_get_template(templates[i]);
        ASSERT(template != NULL, "Template should not be NULL");
    }

    TEST_PASS();
    return true;
}

bool test_shader_formatting(void) {
    TEST_START("Shader Formatting");

    const char *ugly = "void main(){gl_FragColor=vec4(1.0);}";

    char *formatted = shader_format_source(ugly);
    ASSERT(formatted != NULL, "Formatted output should not be NULL");

    printf("Original:\n%s\n", ugly);
    printf("\nFormatted:\n%s\n", formatted);

    free(formatted);

    TEST_PASS();
    return true;
}

bool test_line_numbers(void) {
    TEST_START("Add Line Numbers");

    char *numbered = shader_add_line_numbers(test_shadertoy, 1);
    ASSERT(numbered != NULL, "Numbered output should not be NULL");

    printf("Shader with line numbers:\n%s\n", numbered);

    free(numbered);

    TEST_PASS();
    return true;
}

bool test_performance_estimation(void) {
    TEST_START("Performance Estimation");

    int score = shader_estimate_performance(test_shadertoy);
    printf("Performance score: %d (0=best, 100=worst)\n", score);

    ASSERT(score >= 0 && score <= 100, "Score should be in valid range");

    TEST_PASS();
    return true;
}

bool test_shader_description(void) {
    TEST_START("Generate Shader Description");

    char *desc = shader_generate_description(test_shadertoy);
    ASSERT(desc != NULL, "Description should not be NULL");

    printf("Description: %s\n", desc);

    free(desc);

    TEST_PASS();
    return true;
}

bool test_version_detection(void) {
    TEST_START("GLSL Version Detection");

    int version1 = shader_detect_version(test_raw_fragment);
    printf("Raw fragment shader version: %d\n", version1);
    ASSERT(version1 == 100, "Should detect version 100");

    const char *es3_shader = "#version 300 es\n void main() {}";
    int version2 = shader_detect_version(es3_shader);
    printf("ES 3.0 shader version: %d\n", version2);
    ASSERT(version2 == 300, "Should detect version 300");

    TEST_PASS();
    return true;
}

bool test_uniform_extraction(void) {
    TEST_START("Extract Uniforms");

    const char *shader_with_uniforms =
        "uniform float myFloat;\n"
        "uniform vec2 myVec2;\n"
        "uniform sampler2D myTexture;\n"
        "void main() {}\n";

    char **names = NULL;
    char **types = NULL;
    size_t count = shader_extract_uniforms(shader_with_uniforms, &names, &types);

    printf("Found %zu uniforms:\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("  %s %s\n", types[i], names[i]);
    }

    ASSERT(count == 3, "Should find 3 uniforms");

    shader_free_uniforms(names, types, count);

    TEST_PASS();
    return true;
}

bool test_minification(void) {
    TEST_START("Shader Minification");

    const char *verbose =
        "// Comment\n"
        "void main() {\n"
        "    /* Multi-line\n"
        "       comment */\n"
        "    gl_FragColor = vec4(1.0);\n"
        "}\n";

    char *minified = shader_minify(verbose);
    ASSERT(minified != NULL, "Minified output should not be NULL");

    printf("Original size: %zu bytes\n", strlen(verbose));
    printf("Minified size: %zu bytes\n", strlen(minified));
    printf("Minified: %s\n", minified);

    size_t estimated = shader_estimate_size(verbose);
    printf("Estimated size: %zu bytes\n", estimated);

    free(minified);

    TEST_PASS();
    return true;
}

/* Main test runner */
int main(int argc, char *argv[]) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║       NeoWall Shader Library - Test Suite               ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    // Note: Some tests require OpenGL context, so they may not work
    // in a standalone environment. This is primarily for API testing.
    printf("\nNote: Compilation tests require OpenGL context.\n");
    printf("This test focuses on utility functions that don't need GL.\n");

    // Run tests (skip GL-dependent tests if no context)
    bool has_gl = false; // Would need proper GL context detection

    if (has_gl) {
        test_compile_shadertoy();
        test_compile_raw_fragment();
        test_compile_invalid();
        test_custom_options();
    } else {
        printf("\n⚠ Skipping OpenGL-dependent compilation tests\n");
    }

    // Run utility tests (don't require GL)
    test_shader_validation();
    test_shader_statistics();
    test_shader_templates();
    test_shader_formatting();
    test_line_numbers();
    test_performance_estimation();
    test_shader_description();
    test_version_detection();
    test_uniform_extraction();
    test_minification();

    // Print summary
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                          ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Tests Run:    %-3d                                       ║\n", results.tests_run);
    printf("║  Tests Passed: %-3d                                       ║\n", results.tests_passed);
    printf("║  Tests Failed: %-3d                                       ║\n", results.tests_failed);
    printf("╚══════════════════════════════════════════════════════════╝\n");

    if (results.tests_failed == 0) {
        printf("\n✓ All tests passed! 🎉\n\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n✗ Some tests failed.\n\n");
        return EXIT_FAILURE;
    }
}
