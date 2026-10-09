//! Product identities are selected at compile time, never by downloaded metadata.
pub const MINI: bool = cfg!(feature = "mini");
pub const PLATFORM: &str = if cfg!(target_arch = "aarch64") {
    "windows-arm64"
} else {
    "windows-x64"
};

pub fn validate_platform(platform: &str) -> crate::Result<()> {
    crate::error::require(
        matches!(platform, "windows-x64" | "windows-arm64"),
        "unsupported_update_release",
        "Unsupported update release",
    )
}

pub fn package_prefix(platform: &str) -> String {
    format!("setup/{PRODUCT}_{platform}-")
}
macro_rules! identity {
    ($name:ident, $full:literal, $mini:literal) => {
        pub const $name: &str = if MINI { $mini } else { $full };
    };
}
identity!(PRODUCT, "clip-shot", "clip-shot-mini");
identity!(PRODUCT_NAME, "Clip", "Clip Mini");
identity!(REGISTRY_NAME, "ClipShot", "ClipShotMini");
identity!(APP_PATH, "bin/clip_shot.exe", "bin/clip_shot_mini.exe");
identity!(
    UPDATER_PATH,
    "bin/clip-shot-updater.exe",
    "bin/clip-shot-mini-updater.exe"
);
identity!(
    UPDATER_NONWINDOWS_PATH,
    "bin/clip-shot-updater",
    "bin/clip-shot-mini-updater"
);
identity!(UPDATER_NAME, "clip-shot-updater", "clip-shot-mini-updater");
identity!(
    INSTALLATION_RECORD,
    "clip-shot-installation.json",
    "clip-shot-mini-installation.json"
);
identity!(UPDATE_WORK, ".clip-shot-update", ".clip-shot-mini-update");
identity!(SHARE_PREFIX, "share/clip-shot/", "share/clip-shot-mini/");
pub const PACKAGE_PREFIX: &str = match (MINI, cfg!(target_arch = "aarch64")) {
    (false, false) => "setup/clip-shot_windows-x64-",
    (true, false) => "setup/clip-shot-mini_windows-x64-",
    (false, true) => "setup/clip-shot_windows-arm64-",
    (true, true) => "setup/clip-shot-mini_windows-arm64-",
};
pub const FEED_NAME: &str = match (MINI, cfg!(target_arch = "aarch64")) {
    (false, false) => "latest-version.json",
    (true, false) => "latest-version-mini.json",
    (false, true) => "latest-version-windows-arm64.json",
    (true, true) => "latest-version-mini-windows-arm64.json",
};
identity!(TEMP_PREFIX, "clip-shot-updater-", "clip-shot-mini-updater-");
identity!(
    MARKER_PATH,
    "bin/__data_directory",
    "bin/__mini_data_directory"
);
identity!(
    INSTALL_KEY,
    "Software\\Clip Apps\\ClipShot",
    "Software\\Clip Apps\\ClipShotMini"
);
identity!(
    UNINSTALL_KEY,
    "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ClipShot",
    "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ClipShotMini"
);

pub fn product_matches(value: Option<&serde_json::Value>) -> bool {
    match value {
        None => !MINI,
        Some(value) => value.as_str() == Some(PRODUCT),
    }
}

pub fn installation_variant(variant: &str) -> bool {
    matches!(variant, "online" | "portable") || (!MINI && variant == "offline")
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn product_binding_accepts_only_this_product_and_legacy_full_records() {
        assert!(product_matches(Some(&json!(PRODUCT))));
        assert!(!product_matches(Some(&json!(if MINI {
            "clip-shot"
        } else {
            "clip-shot-mini"
        }))));
        assert!(!product_matches(Some(&json!(null))));
        assert_eq!(product_matches(None), !MINI);
        assert_eq!(installation_variant("offline"), !MINI);
        assert!(installation_variant("online"));
        assert!(installation_variant("portable"));
    }
}
