//! Compile-time product identity and the reduced Mini MCP contract.
pub const MINI: bool = cfg!(feature = "mini");
pub const PRODUCT: &str = if MINI { "clip-shot-mini" } else { "clip-shot" };
pub const PRODUCT_NAME: &str = if MINI { "Clip Mini" } else { "Clip" };
pub const MCP_NAME: &str = if MINI {
    "clip-shot-mini-mcp"
} else {
    "clip-shot-mcp"
};
pub const REGISTRY_NAME: &str = if MINI { "ClipShotMini" } else { "ClipShot" };
pub const APP_NAME: &str = if MINI { "clip_shot_mini" } else { "clip_shot" };

pub fn method_enabled(name: &str) -> bool {
    !MINI
        || !matches!(
            name,
            "clip_shot_screenshot_translate"
                | "clip_shot_models_list"
                | "clip_shot_models_update"
                | "clip_shot_credentials_set"
                | "clip_shot_translation_catalog"
                | "clip_shot_translation_start"
        )
}

pub fn input_enabled(name: &str, input: &serde_json::Value) -> bool {
    if !method_enabled(name) {
        return false;
    }
    if !MINI {
        return true;
    }
    match name {
        "clip_shot_screenshot_recognize" | "clip_shot_document_recognize" => {
            input.get("kind").and_then(serde_json::Value::as_str) == Some("text")
        }
        "clip_shot_pinned_edit" => {
            let action = input.get("action").and_then(serde_json::Value::as_str);
            action != Some("translate")
                && (action != Some("recognize")
                    || input
                        .get("payload")
                        .and_then(|p| p.get("kind"))
                        .and_then(serde_json::Value::as_str)
                        == Some("text"))
        }
        _ => true,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn retained_ids_and_recognition_modes_follow_the_compiled_edition() {
        assert!(method_enabled("clip_shot_document_recognize"));
        assert_eq!(method_enabled("clip_shot_translation_start"), !MINI);
        for kind in ["table", "qr", "latex", "markdown", "html"] {
            assert_eq!(
                input_enabled("clip_shot_document_recognize", &json!({"kind":kind})),
                !MINI
            );
            assert_eq!(
                input_enabled(
                    "clip_shot_pinned_edit",
                    &json!({"action":"recognize","payload":{"kind":kind}})
                ),
                !MINI
            );
        }
        assert!(input_enabled(
            "clip_shot_document_recognize",
            &json!({"kind":"text"})
        ));
    }
}
