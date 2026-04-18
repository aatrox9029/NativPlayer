#include "tests/scenario_runner_internal.h"

namespace velo::tests {

void TestPlaylistHelpers(ScenarioResult& result) {
    const auto root = std::filesystem::temp_directory_path() / "nativplayer-tests" / "playlist";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root, error);

    const auto fileA = root / "A.mp4";
    const auto fileB = root / "B.mkv";
        const auto fileC = root / "C.flac";
    const auto subtitle = root / "B.srt";
        const auto lyric = root / "C.lrc";
    std::ofstream(fileA.string()).put('a');
    std::ofstream(fileB.string()).put('b');
        std::ofstream(fileC.string()).put('c');
    std::ofstream(subtitle.string()).put('s');
        std::ofstream(lyric.string()).put('l');

    const auto playlist = velo::app::BuildPlaylistFromFile(fileB);
        Expect(playlist.items.size() == 3, "playlist helper scans sibling playable media", result);
    Expect(playlist.currentIndex == 1, "playlist helper selects current file index", result);

    const auto folderPlaylist = velo::app::BuildPlaylistFromFolder(root);
        Expect(folderPlaylist.items.size() == 3, "folder playlist collects playable media", result);
    Expect(folderPlaylist.currentIndex == 0, "folder playlist starts at first item", result);

    const auto foundSubtitle = velo::app::FindSidecarSubtitle(fileB);
    Expect(foundSubtitle.has_value() && foundSubtitle->filename() == subtitle.filename(),
           "playlist helper finds same-name subtitle", result);
    Expect(velo::app::IsSubtitleFile(subtitle), "subtitle helper detects subtitle file", result);
        Expect(velo::app::IsSubtitleFile(lyric), "subtitle helper detects extended subtitle formats", result);
        Expect(velo::app::IsPlayableMediaFile(fileC), "media helper accepts audio-only playable files", result);

        const auto tempPlaylist = velo::app::BuildTemporaryPlaylist({fileB.wstring(), fileA.wstring(), fileC.wstring(), fileB.wstring(), subtitle.wstring()});
        Expect(tempPlaylist.items.size() == 3, "temporary playlist filters duplicates and subtitles", result);
    Expect(tempPlaylist.currentIndex == 0, "temporary playlist starts at first item", result);

    const auto nextTempPlaylist = velo::app::BuildTemporaryPlaylist({fileA.wstring()});
    Expect(nextTempPlaylist.items.size() == 1 && nextTempPlaylist.items.front() == fileA.wstring(),
           "temporary playlist rebuilds cleanly on repeated drops", result);
    Expect(nextTempPlaylist.currentIndex == 0, "temporary playlist resets index on repeated drops", result);

        const auto childFolder = root / "Season 1";
        std::filesystem::create_directories(childFolder, error);
        const auto childVideo = childFolder / "C.mp4";
        std::ofstream(childVideo.string()).put('c');
        const auto quickBrowseRoot = velo::app::BuildQuickBrowseCatalog(fileB.wstring());
        Expect(quickBrowseRoot.EntryCount() >= 3, "quick browse lists folders and videos", result);
        Expect(!quickBrowseRoot.Empty() && quickBrowseRoot.EntryAt(0).kind == velo::app::QuickBrowseEntryKind::Folder,
            "quick browse places folders at top", result);
        Expect(quickBrowseRoot.activeEntryIndex >= 0 && quickBrowseRoot.EntryAt(quickBrowseRoot.activeEntryIndex).active,
            "quick browse marks active video", result);

        const auto quickBrowseChild = velo::app::BuildQuickBrowseCatalog(fileB.wstring(), childFolder.wstring());
        Expect(!quickBrowseChild.Empty() && quickBrowseChild.EntryAt(0).kind == velo::app::QuickBrowseEntryKind::NavigateUp,
            "quick browse exposes navigate up item in subfolder", result);
        bool foundChildVideo = false;
        for (int index = 0; index < static_cast<int>(quickBrowseChild.EntryCount()); ++index) {
            const auto entry = quickBrowseChild.EntryAt(index);
            if (entry.kind == velo::app::QuickBrowseEntryKind::Video && entry.path == childVideo.wstring()) {
                foundChildVideo = true;
                break;
            }
        }
        Expect(foundChildVideo,
            "quick browse shows videos in selected folder", result);

        const auto quickBrowsePreservedRoot = velo::app::BuildQuickBrowseCatalog(childVideo.wstring(), childFolder.wstring(), root.wstring());
        Expect(!quickBrowsePreservedRoot.Empty() && quickBrowsePreservedRoot.EntryAt(0).kind == velo::app::QuickBrowseEntryKind::NavigateUp,
            "quick browse keeps navigate up item after child-folder playback", result);
        Expect(quickBrowsePreservedRoot.rootFolder == root.wstring(), "quick browse preserves original root folder while panel stays open", result);
}

void TestPlaylistStorage(ScenarioResult& result) {
    const auto root = std::filesystem::temp_directory_path() / "nativplayer-tests" / "playlist-storage";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root, error);

    std::vector<std::wstring> items = {L"A.mp4", L"B.mkv", L"C.mp4"};
    Expect(velo::app::MovePlaylistItem(items, 2, 1), "playlist storage moves item", result);
    Expect(items[1] == L"C.mp4", "playlist storage preserves moved order", result);
    Expect(velo::app::RemovePlaylistItem(items, 1), "playlist storage removes item", result);
    Expect(items.size() == 2, "playlist storage shrinks after remove", result);
    velo::app::InsertIntoPlaylist(items, {L"X.mp4", L"Y.mp4"}, 1);
    Expect(items.size() == 4 && items[1] == L"X.mp4", "playlist storage inserts items", result);

    const auto playlistPath = root / "sample.m3u8";
    Expect(velo::app::SavePlaylist(playlistPath, items), "playlist storage saves m3u", result);
    const auto loaded = velo::app::LoadPlaylist(playlistPath);
    Expect(loaded.size() == items.size(), "playlist storage loads m3u", result);
}

void TestMediaSourceHelpers(ScenarioResult& result) {
    Expect(velo::app::IsLikelyUrl(L"https://example.com/live.m3u8"), "media source detects URL", result);
    Expect(velo::app::DetectMediaSourceKind(L"movie.iso") == velo::app::MediaSourceKind::DiscImage,
           "media source detects disc image", result);
    Expect(velo::app::DetectMediaSourceKind(L"dvd://1") == velo::app::MediaSourceKind::DiscProtocol,
           "media source detects disc protocol", result);
    Expect(velo::app::DetectMediaSourceKind(L"playlist.m3u8") == velo::app::MediaSourceKind::PlaylistContainer,
           "media source detects playlist container", result);
    Expect(velo::app::DetectMediaSourceKind(L"radio.xspf") == velo::app::MediaSourceKind::PlaylistContainer,
        "media source detects extended playlist containers", result);
    Expect(!velo::app::SupportsDirectPlayback(L"movie.iso"), "media source blocks direct iso playback", result);
}

void TestMediaHistoryHelpers(ScenarioResult& result) {
    std::vector<velo::config::RecentItem> recentItems;
    velo::app::RememberRecentItem(recentItems, L"movie.mkv", 100);
    velo::app::RememberRecentItem(recentItems, L"https://example.com/live.m3u8", 101);
    velo::app::TogglePinnedRecent(recentItems, L"movie.mkv");
    Expect(recentItems.size() == 2 && recentItems.front().path == L"movie.mkv" && recentItems.front().pinned,
           "media history pins recent item", result);
    velo::app::ClearUnpinnedRecentItems(recentItems);
    Expect(recentItems.size() == 1, "media history clears unpinned items", result);

    std::vector<velo::config::HistoryEntry> history;
    velo::app::RememberHistoryEntry(history, L"movie.mkv", L"Movie", 88.0, false, 200);
    Expect(history.size() == 1 && history.front().positionSeconds == 88.0, "media history records playback history", result);

    std::vector<velo::config::BookmarkEntry> bookmarks;
    velo::app::RememberBookmark(bookmarks, L"movie.mkv", 12.0, L"Intro");
    Expect(bookmarks.size() == 1 && bookmarks.front().label == L"Intro", "media history records bookmark", result);

    const auto recentPaths = velo::app::BuildRecentPaths(recentItems);
    Expect(recentPaths.size() == 1 && recentPaths.front() == L"movie.mkv", "media history builds recent path list", result);
}


void RunPlaylistScenarios(ScenarioResult& result) {
    TestPlaylistHelpers(result);
    TestPlaylistStorage(result);
    TestMediaSourceHelpers(result);
    TestMediaHistoryHelpers(result);
}

}  // namespace velo::tests

