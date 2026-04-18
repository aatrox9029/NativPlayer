#include "tests/scenario_runner_internal.h"

#include <cstring>

#include "common/portable_executable.h"
#include "playback/mpv_runtime_probe.h"

namespace velo::tests {
namespace {

void WritePortableExecutableStub(const std::filesystem::path& path, const WORD machine) {
    std::vector<unsigned char> bytes(512, 0);

    IMAGE_DOS_HEADER dosHeader{};
    dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
    dosHeader.e_lfanew = 0x80;
    std::memcpy(bytes.data(), &dosHeader, sizeof(dosHeader));

    const DWORD signature = IMAGE_NT_SIGNATURE;
    std::memcpy(bytes.data() + 0x80, &signature, sizeof(signature));

    IMAGE_FILE_HEADER fileHeader{};
    fileHeader.Machine = machine;
    std::memcpy(bytes.data() + 0x80 + sizeof(signature), &fileHeader, sizeof(fileHeader));

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

}  // namespace

void TestDiagnosticsHelpers(ScenarioResult& result) {
    const std::string systemSummary = velo::diagnostics::BuildSystemSummary();
    Expect(systemSummary.find("monitor_count=") != std::string::npos, "system report includes monitor count", result);
    Expect(systemSummary.find("process_arch=") != std::string::npos, "diagnostics bundle includes process architecture", result);
    Expect(systemSummary.find("process_working_set_mb=") != std::string::npos, "system report includes process working set", result);
    Expect(systemSummary.find("mpv_loaded_arch=") != std::string::npos, "diagnostics bundle includes loaded mpv architecture", result);
    Expect(systemSummary.find("mpv_candidate_arch=") != std::string::npos,
           "diagnostics bundle includes detected mpv candidate architecture", result);
    Expect(velo::diagnostics::ClassifyPlaybackFailure("Decoder init failed") == "decoder_unavailable",
           "diagnostics classify decoder failures", result);
    Expect(velo::diagnostics::ClassifyPlaybackFailure("audio device unavailable") == "audio_output_failure",
           "diagnostics classify audio failures", result);

    const auto currentProcess = velo::common::CurrentProcessPath();
    const auto outputDirectory = currentProcess.parent_path();

    velo::common::PortableExecutableInfo currentProcessInfo;
    Expect(velo::common::TryReadPortableExecutableInfo(currentProcess, currentProcessInfo) &&
               currentProcessInfo.architecture == velo::common::BinaryArchitecture::X64,
           "headless artifact reports x64 machine type", result);

    const auto playerArtifact = outputDirectory / "nativplayer.exe";
    velo::common::PortableExecutableInfo playerInfo;
    Expect(velo::common::TryReadPortableExecutableInfo(playerArtifact, playerInfo) &&
               playerInfo.architecture == velo::common::BinaryArchitecture::X64,
           "build artifact reports x64 machine type", result);

    const auto repoRoot = outputDirectory.parent_path();
    const auto canonicalRuntime = repoRoot / "runtime" / "win64" / "libmpv-2.dll";
    const auto runtimeProbe = std::filesystem::exists(canonicalRuntime)
                                  ? velo::playback::ProbeMpvRuntimeLibrary(canonicalRuntime, L"libmpv-2.dll")
                                  : velo::playback::ProbeFirstMpvRuntimeCandidate();
    Expect(runtimeProbe.found && runtimeProbe.libraryImage.valid &&
               runtimeProbe.libraryImage.architecture == velo::common::BinaryArchitecture::X64,
           "runtime libmpv dll reports x64 machine type", result);

    const auto mismatchRoot = std::filesystem::temp_directory_path() / "nativplayer-tests" / "mpv-arch-mismatch";
    std::error_code mismatchError;
    std::filesystem::remove_all(mismatchRoot, mismatchError);
    std::filesystem::create_directories(mismatchRoot, mismatchError);
    const auto fakeX86Dll = mismatchRoot / "fake-libmpv-x86.dll";
    WritePortableExecutableStub(fakeX86Dll, IMAGE_FILE_MACHINE_I386);
    const auto mismatchProbe = velo::playback::ProbeMpvRuntimeLibrary(fakeX86Dll, L"libmpv-2.dll");
    Expect(mismatchProbe.architectureMismatch, "mpv loader flags incompatible library architecture", result);
    Expect(mismatchProbe.diagnosticMessage.find("Incompatible libmpv architecture") != std::string::npos &&
               mismatchProbe.diagnosticMessage.find("process=x64") != std::string::npos &&
               mismatchProbe.diagnosticMessage.find("library=x86") != std::string::npos,
           "mpv loader reports incompatible architecture clearly", result);

    velo::diagnostics::StartupMetrics metrics;
    metrics.Mark("window");
    metrics.MarkFirstFrame();
    const auto exportRoot = std::filesystem::temp_directory_path() / "nativplayer-tests" / "diagnostics-export";
    std::error_code error;
    std::filesystem::remove_all(exportRoot, error);
    std::filesystem::create_directories(exportRoot, error);
    const auto configPath = exportRoot / "settings.ini";
    const auto logPath = exportRoot / "session.log";
    std::ofstream(configPath.string()) << "config_version=2\n";
    std::ofstream(logPath.string()) << "test log\n";
    const std::wstring versionLabel = velo::app::AppVersionLabel();
    const auto bundle = velo::diagnostics::ExportDiagnosticsBundle(exportRoot, configPath, {}, logPath,
                                                                   std::string(versionLabel.begin(), versionLabel.end()),
                                                                   metrics, {{"decoder_unavailable", 3}});
    Expect(!bundle.empty() && std::filesystem::exists(bundle / "report.txt"), "diagnostics export writes bundle", result);
}

void TestBenchmarkAndReleaseGate(ScenarioResult& result) {
    velo::diagnostics::StartupMetrics metrics;
    metrics.Mark("window");
    metrics.MarkFirstFrame();
    velo::ui::PlayerState state;
    state.isLoaded = true;
    state.cacheBufferSeconds = 1.2;
    state.droppedFrameCount = 2;
    state.decoderDropCount = 0;
    const auto benchmark = velo::diagnostics::BuildBenchmarkReport(metrics, state);
    Expect(!benchmark.text.empty(), "benchmark report builds output", result);
    Expect(benchmark.text.find("process_working_set_mb=") != std::string::npos, "benchmark report includes working set", result);
    Expect(benchmark.text.find("process_private_bytes_mb=") != std::string::npos, "benchmark report includes private bytes", result);

    const auto benchmarkPath = std::filesystem::temp_directory_path() / "nativplayer-tests" / "benchmark-latest.log";
    Expect(velo::diagnostics::WriteBenchmarkReport(benchmarkPath, benchmark), "benchmark report writes file", result);

    velo::diagnostics::ReleaseGateInput gateInput;
    gateInput.headlessAutomationPassed = true;
    gateInput.uiSmokePassed = true;
    gateInput.benchmark = benchmark;
    const auto gateResult = velo::diagnostics::EvaluateReleaseGate(gateInput);
    Expect(gateResult.passed, "release gate passes healthy inputs", result);
}

void TestSessionRecovery(ScenarioResult& result) {
    const auto root = std::filesystem::temp_directory_path() / "nativplayer-tests" / "session-recovery";
    std::error_code error;
    std::filesystem::remove_all(root, error);

    velo::diagnostics::SessionRecoveryTracker firstRun(root);
    Expect(firstRun.PrepareForStartup(), "session recovery creates startup marker", result);
    Expect(!firstRun.HadPreviousUncleanExit(), "session recovery first run is clean", result);

    velo::diagnostics::SessionRecoveryTracker secondRun(root);
    Expect(secondRun.PrepareForStartup(), "session recovery can re-open marker", result);
    Expect(secondRun.HadPreviousUncleanExit(), "session recovery detects unclean exit", result);
    secondRun.MarkCleanExit();
    Expect(!std::filesystem::exists(secondRun.MarkerPath()), "session recovery clears marker on clean exit", result);
}

void TestSubtitleDiagnosticsSnapshot(ScenarioResult& result) {
    velo::config::AppConfig config = velo::config::DefaultAppConfig();
    config.showDebugInfo = true;
    config.subtitlePositionPreset = L"middle";
    config.subtitleVerticalMargin = 38;
    const auto layout = velo::playback::BuildSubtitleLayout(config);
    const std::string snapshot = velo::playback::BuildSubtitleDiagnosticsSnapshot(config, layout);

        Expect(layout.marginLeft == layout.marginRight, "subtitle diagnostics keeps centered horizontal margins", result);
        Expect(snapshot.find("horizontal_offset=0") != std::string::npos,
            "subtitle diagnostics snapshot records cleared horizontal offset", result);
    Expect(layout.verticalPosition == 38, "subtitle diagnostics uses the configured vertical subtitle position", result);
    Expect(snapshot.find("margin_left=") != std::string::npos && snapshot.find("style_overrides=") != std::string::npos,
           "subtitle diagnostics snapshot includes computed mpv values", result);

    const auto logRoot = std::filesystem::temp_directory_path() / "nativplayer-tests" / "subtitle-diagnostics";
    std::error_code error;
    std::filesystem::remove_all(logRoot, error);
    std::filesystem::create_directories(logRoot, error);
    velo::diagnostics::Logger logger(logRoot);
    logger.Info("subtitle apply snapshot: " + snapshot);
    std::ifstream input(logger.SessionLogPath());
    std::stringstream buffer;
    buffer << input.rdbuf();
    const std::string logContents = buffer.str();
}

void RunDiagnosticsScenarios(ScenarioResult& result) {
    AppendHeadlessTrace("diagnostics:helpers:start");
    TestDiagnosticsHelpers(result);
    AppendHeadlessTrace("diagnostics:helpers:end");
    AppendHeadlessTrace("diagnostics:benchmark:start");
    TestBenchmarkAndReleaseGate(result);
    AppendHeadlessTrace("diagnostics:benchmark:end");
    AppendHeadlessTrace("diagnostics:session_recovery:start");
    TestSessionRecovery(result);
    AppendHeadlessTrace("diagnostics:session_recovery:end");
    AppendHeadlessTrace("diagnostics:subtitle_snapshot:start");
    TestSubtitleDiagnosticsSnapshot(result);
    AppendHeadlessTrace("diagnostics:subtitle_snapshot:end");
}

}  // namespace velo::tests

