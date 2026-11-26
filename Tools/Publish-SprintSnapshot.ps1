#Requires -Version 7.0
param(
 [string]$SourceCommit='HEAD',
 [string]$RemoteBranch='development/thursday-2026-11-26',
 [string]$PublishRef='refs/heads/sprint-publish/thursday-2026-11-26',
 [Parameter(Mandatory)][string]$Verification,
 [string]$ProjectRoot=(Join-Path $PSScriptRoot '..')
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath $ProjectRoot).Path
& git -C $root check-ref-format "refs/heads/$RemoteBranch"
if($LASTEXITCODE -ne 0){throw 'Invalid remote branch'}
$source=(& git -C $root rev-parse --verify "$SourceCommit^{commit}").Trim()
if($LASTEXITCODE -ne 0){throw 'Source commit missing'}
$parent=(& git -C $root rev-parse --verify $PublishRef).Trim()
if($LASTEXITCODE -ne 0){throw 'Known publication parent missing; do not guess or overwrite remote history'}
$remote=& git -C $root ls-remote --heads origin $RemoteBranch
if($LASTEXITCODE -ne 0 -or !$remote -or ($remote -split '\s+')[0] -ne $parent){throw 'Remote branch changed; inspect before publishing'}
$tree=(& git -C $root rev-parse "$source^{tree}").Trim()
if($LASTEXITCODE -ne 0){throw 'Source tree missing'}
$subject=(& git -C $root log -1 --format=%s $source).Trim()
$message="Exact committed source snapshot: $source. $Verification. Publishes on existing remote snapshot history to avoid transferring rewritten recovery ancestors. The local source history is preserved; no force push or history rewrite."
$snapshot=(& git -C $root commit-tree $tree -p $parent -m $subject -m $message).Trim()
if($LASTEXITCODE -ne 0){throw 'Publication commit failed'}
& git -C $root diff --quiet $source $snapshot
if($LASTEXITCODE -ne 0){throw 'Publication tree differs from verified source'}
& git -C $root -c http.version=HTTP/1.1 push origin "${snapshot}:refs/heads/$RemoteBranch"
if($LASTEXITCODE -ne 0){throw 'Push failed; local source and previous publication pointer are preserved'}
$actual=& git -C $root ls-remote --heads origin $RemoteBranch
if($LASTEXITCODE -ne 0 -or ($actual -split '\s+')[0] -ne $snapshot){throw 'Remote publication verification failed'}
& git -C $root update-ref $PublishRef $snapshot $parent
if($LASTEXITCODE -ne 0){throw 'Published successfully but local publication pointer changed; inspect before next push'}
[pscustomobject]@{sourceCommit=$source;publishedCommit=$snapshot;branch=$RemoteBranch;identicalTree=$true}
