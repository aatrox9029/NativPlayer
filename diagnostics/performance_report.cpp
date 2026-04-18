#include "diagnostics/performance_report.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include "diagnostics/process_memory.h"

namespace velo::diagnostics {

BenchmarkReport BuildBenchmarkReport(const StartupMetrics& metrics, const velo::ui::PlayerState& state) {
    BenchmarkReport report;
    report.text = "[startup]\n" + metrics.BuildInlineSummary() + "\n[playback]\n";
    const ProcessMemorySnapshot memory = QueryCurrentProcessMemorySnapshot();

    const auto append = [&](std::string name, double value, std::string unit, double threshold, bool lowerIsBetter) {
        const bool pass = lowerIsBetter ? value <= threshold : value >= threshold;
        report.samples.push_back({std::move(name), value, std::move(unit), threshold, pass});
        report.passed = report.passed && pass;
    };

    append("subtitle_delay_abs", std::abs(state.subtitleDelaySeconds), "s", 0.75, true);
    append("audio_delay_abs", std::abs(state.audioDelaySeconds), "s", 0.75, true);
    append("av_sync_abs", std::abs(state.avSyncSeconds), "s", 0.08, true);
    append("dropped_frames", static_cast<double>(state.droppedFrameCount), "frames", 30.0, true);
    append("decoder_dropped_frames", static_cast<double>(state.decoderDropCount), "frames", 10.0, true);
    append("cache_buffer_seconds", state.cacheBufferSeconds, "s", 0.25, false);
    if (state.packetQueueDepth > 0 || state.frameQueueDepth > 0) {
        append("packet_queue_depth", static_cast<double>(state.packetQueueDepth), "packets", 160.0, true);
        append("frame_queue_depth", static_cast<double>(state.frameQueueDepth), "frames", 16.0, true);
    }
    if (state.audioBufferSeconds > 0.0) {
        append("audio_buffer_seconds", state.audioBufferSeconds, "s", 0.03, false);
    }
    if (state.audioDeviceDriven || state.audioDeviceLatencyMs > 0.0) {
        append("audio_device_latency_ms", state.audioDeviceLatencyMs, "ms", 80.0, true);
    }
    if (state.decodeLatencyMs > 0.0) {
        append("decode_latency_ms", state.decodeLatencyMs, "ms", 40.0, true);
    }
    if (state.renderLatencyMs > 0.0) {
        append("render_latency_ms", state.renderLatencyMs, "ms", 25.0, true);
    }
    if (state.presentJitterMs > 0.0) {
        append("present_jitter_ms", state.presentJitterMs, "ms", 12.0, true);
    }
    if (state.startupLatencyMs > 0.0) {
        append("startup_latency_ms", state.startupLatencyMs, "ms", 1500.0, true);
    }
    if (state.seekLatencyMs > 0.0) {
        append("seek_latency_ms", state.seekLatencyMs, "ms", 500.0, true);
    }
    append("audio_clock_drift_ms", std::abs(state.audioClockDriftMs), "ms", 60.0, true);
    append("audio_clock_peak_drift_ms", state.audioClockPeakDriftMs, "ms", 120.0, true);
    if (state.nativeMemoryUsedMb > 0.0) {
        append("native_memory_used_mb", state.nativeMemoryUsedMb, "MB", 256.0, true);
        append("native_memory_peak_mb", state.nativeMemoryPeakMb, "MB", 384.0, true);
    }
    if (state.gpuMemoryUsedMb > 0.0 || state.gpuMemoryPeakMb > 0.0) {
        append("gpu_memory_used_mb", state.gpuMemoryUsedMb, "MB", 384.0, true);
        append("gpu_memory_peak_mb", state.gpuMemoryPeakMb, "MB", 512.0, true);
    }
    if (state.texturePoolHits > 0 || state.texturePoolMisses > 0) {
        append("texture_pool_misses", static_cast<double>(state.texturePoolMisses), "allocs", 32.0, true);
        append("swapchain_present_count", static_cast<double>(state.swapchainPresentCount), "presents", 0.0, false);
    }
    if (state.shaderPassCount > 0 || state.shaderCacheHits > 0 || state.shaderCacheMisses > 0) {
        append("shader_pass_count", static_cast<double>(state.shaderPassCount), "passes", 2.0, false);
        append("shader_cache_misses", static_cast<double>(state.shaderCacheMisses), "pipelines", 16.0, true);
    }
    if (state.shaderDrawCount > 0) {
        append("shader_draw_count", static_cast<double>(state.shaderDrawCount), "draws", 1.0, false);
    }
    if (state.subtitleAtlasPageCount > 0 || state.subtitleAtlasGlyphCount > 0 || state.subtitleAtlasMisses > 0) {
        append("subtitle_atlas_pages", static_cast<double>(state.subtitleAtlasPageCount), "pages", 8.0, true);
        append("subtitle_atlas_glyphs", static_cast<double>(state.subtitleAtlasGlyphCount), "glyphs", 1.0, false);
        append("subtitle_atlas_misses", static_cast<double>(state.subtitleAtlasMisses), "glyphs", 256.0, true);
    }
    if (state.subtitleCompositeCount > 0) {
        append("subtitle_composite_count", static_cast<double>(state.subtitleCompositeCount), "draws", 1.0, false);
    }
    if (state.rebufferCount > 0) {
        append("rebuffer_count", static_cast<double>(state.rebufferCount), "events", 4.0, true);
    }
    if (state.audioUnderrunCount > 0 || state.audioClockCorrectionCount > 0) {
        append("audio_underrun_count", static_cast<double>(state.audioUnderrunCount), "events", 4.0, true);
        append("audio_clock_corrections", static_cast<double>(state.audioClockCorrectionCount), "events", 24.0, true);
    }
    if (memory.available) {
        append("process_working_set_mb", memory.workingSetMb, "MB", 768.0, true);
        append("process_private_bytes_mb", memory.privateBytesMb, "MB", 1024.0, true);
        append("process_peak_working_set_mb", memory.peakWorkingSetMb, "MB", 1024.0, true);
    }

    std::ostringstream output;
    output << report.text;
    for (const auto& sample : report.samples) {
        output << sample.name << '=' << sample.value << sample.unit << " threshold=" << sample.threshold
               << (sample.pass ? " pass" : " fail") << '\n';
    }
    output << "renderer_backend=" << state.renderer << '\n';
    output << "decode_path=" << state.decodePath << '\n';
    output << "decoder_surface_pool=" << state.decoderSurfacePool << '\n';
    output << "decoder_surface_pool_shared_in_use=" << state.decoderSurfacePoolSharedInUse << '\n';
    output << "decoder_surface_pool_transfer_in_use=" << state.decoderSurfacePoolTransferInUse << '\n';
    output << "decoder_surface_pool_upload_in_use=" << state.decoderSurfacePoolUploadInUse << '\n';
    output << "hwdec_current=" << state.hwdecCurrent << '\n';
    output << "decoder_fallback_active=" << (state.decoderFallbackActive ? 1 : 0) << '\n';
    output << "zero_copy_video_path_active=" << (state.zeroCopyVideoPathActive ? 1 : 0) << '\n';
    output << "gpu_memory_budget_mb=" << state.gpuMemoryBudgetMb << "MB\n";
    report.text = output.str();
    return report;
}

bool WriteBenchmarkReport(const std::filesystem::path& path, const BenchmarkReport& report) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output << report.text;
    return true;
}

}  // namespace velo::diagnostics
