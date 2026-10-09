# Copy to publish-clip-shot-release.local.ps1 (ignored by Git), then fill local settings.
[CmdletBinding()]
param([ValidateSet('Publish', 'Verify')][string]$Operation = 'Publish', [switch]$SkipBuild, [switch]$AuditOnly, [switch]$SkipGitee, [switch]$DeployWebsite, [string]$ReleaseNotesPath, [switch]$WhatIf)
$releaseSettings = @{
    GitHubRepository = 'mg-chao/clip-apps'
    SigningKeyPath = 'C:/private/clip-shot-release/private.pem'
    WebsiteDirectory = 'D:/clip-apps-site'
}
# GITEE_TOKEN is read locally; never store it in this file or GitHub release workflows.
& "$PSScriptRoot/publish-clip-shot-release.ps1" @releaseSettings -Operation $Operation -SkipBuild:$SkipBuild -AuditOnly:$AuditOnly -SkipGitee:$SkipGitee -DeployWebsite:$DeployWebsite -ReleaseNotesPath $ReleaseNotesPath -WhatIf:$WhatIf
