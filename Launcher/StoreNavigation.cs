namespace StarshatterWarsLauncher;
internal static class StoreNavigation
{
    public static Uri? Website(string? value) => Uri.TryCreate(value, UriKind.Absolute, out var uri)
        && uri.Scheme == Uri.UriSchemeHttps && string.IsNullOrEmpty(uri.UserInfo)
        && !string.IsNullOrWhiteSpace(uri.Host) ? uri : null;
}
