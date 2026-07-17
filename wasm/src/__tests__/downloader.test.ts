import { describe, it, expect, vi, beforeEach } from "vitest";
import { fetchBuildInfo, makeDownloadUrl, resolve, downloadEntry } from "../downloader.js";
import type { BuildInfoEntry } from "../downloader.js";

beforeEach(() => {
  vi.restoreAllMocks();
});

describe("fetchBuildInfo", () => {
  it("fetches and parses build info JSON", async () => {
    const mockJson = {
      totalSize: 123456,
      build: 42,
      entries: [
        { path: "/data/data.nos", sha1: "abc123", file: "data.nos", flags: 0, size: 1000, folder: false },
      ],
    };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(
      new Response(JSON.stringify(mockJson), { status: 200 }),
    );
    const info = await fetchBuildInfo("game1", "build42");
    expect(fetch).toHaveBeenCalledWith(
      "https://spark.gameforge.com/api/v1/patching/download/install/game1/default/build42",
      { headers: { "User-Agent": "GameforgeClient/2.8.5", "Origin": "spark://www.gameforge.com" } },
    );
    expect(info.totalSize).toBe(123456);
    expect(info.build).toBe(42);
    expect(info.entries).toHaveLength(1);
    expect(info.entries[0]).toMatchObject({
      path: "/data/data.nos",
      sha1: "abc123",
      file: "data.nos",
      flags: 0,
      size: 1000,
      folder: false,
    });
  });

  it("uses latest URL when buildId is 'latest'", async () => {
    vi.spyOn(globalThis, "fetch").mockResolvedValue(
      new Response(JSON.stringify({ entries: [], totalSize: 0, build: 0 }), { status: 200 }),
    );
    await fetchBuildInfo("mygame", "latest");
    expect(fetch).toHaveBeenCalledWith(
      "https://spark.gameforge.com/api/v1/patching/download/latest/mygame/default",
      expect.objectContaining({ headers: expect.any(Object) }),
    );
  });

  it("throws on network error", async () => {
    vi.spyOn(globalThis, "fetch").mockResolvedValue(new Response(null, { status: 404 }));
    await expect(fetchBuildInfo("g", "b")).rejects.toThrow("Failed to fetch build info");
  });
});

describe("makeDownloadUrl", () => {
  it("constructs URL from entry path", () => {
    const entry: BuildInfoEntry = {
      path: "/v1/patch/nostale/data.nos",
      sha1: "abc",
      file: "data.nos",
      flags: 0,
      size: 100,
      folder: false,
    };
    expect(makeDownloadUrl(entry)).toBe("http://patches.gameforge.com/v1/patch/nostale/data.nos");
  });

  it("handles path without leading slash", () => {
    const entry: BuildInfoEntry = {
      path: "relative/path/file.bin",
      sha1: "",
      file: "file.bin",
      flags: 0,
      size: 50,
      folder: false,
    };
    expect(makeDownloadUrl(entry)).toBe("http://patches.gameforge.comrelative/path/file.bin");
  });
});

describe("resolve", () => {
  const entries: BuildInfoEntry[] = [
    { path: "/a/data.nos", sha1: "1", file: "data.nos", flags: 0, size: 10, folder: false },
    { path: "/b/other.bin", sha1: "2", file: "other.bin", flags: 0, size: 20, folder: false },
    { path: "/c/sub/data.nos", sha1: "3", file: "sub/data.nos", flags: 0, size: 30, folder: false },
    { path: "/d/dup.bin", sha1: "4", file: "dup.bin", flags: 0, size: 40, folder: false },
    { path: "/e/dup.bin", sha1: "5", file: "dup.bin", flags: 0, size: 50, folder: false },
    { path: "/f/dir", sha1: "6", file: "dir", flags: 0, size: 0, folder: true },
    { path: "/g/empty.dat", sha1: "7", file: "", flags: 0, size: 0, folder: false },
    { path: "/h/sub/unique.bin", sha1: "8", file: "sub/unique.bin", flags: 0, size: 60, folder: false },
    { path: "/i/sub/common.bin", sha1: "9", file: "a/common.bin", flags: 0, size: 70, folder: false },
    { path: "/j/other/common.bin", sha1: "10", file: "b/common.bin", flags: 0, size: 80, folder: false },
  ];

  it("returns exact match", () => {
    const result = resolve(entries, "data.nos");
    expect(result).not.toBeNull();
    expect(result!.file).toBe("data.nos");
    expect(result!.path).toBe("/a/data.nos");
  });

  it("returns null for ambiguous exact match", () => {
    const result = resolve(entries, "dup.bin");
    expect(result).toBeNull();
  });

  it("returns bare filename match", () => {
    const result = resolve(entries, "unique.bin");
    expect(result).not.toBeNull();
    expect(result!.file).toBe("sub/unique.bin");
    expect(result!.path).toBe("/h/sub/unique.bin");
  });

  it("returns null for ambiguous bare match", () => {
    const result = resolve(entries, "common.bin");
    expect(result).toBeNull();
  });

  it("returns null when not found", () => {
    const result = resolve(entries, "nonexistent.bin");
    expect(result).toBeNull();
  });

  it("skips folder entries", () => {
    const result = resolve(entries, "dir");
    expect(result).toBeNull();
  });

  it("returns null for empty entries array", () => {
    const result = resolve([], "anything");
    expect(result).toBeNull();
  });
});

describe("downloadEntry", () => {
  it("downloads entry as Uint8Array", async () => {
    vi.spyOn(globalThis, "fetch").mockResolvedValue(
      new Response(new ArrayBuffer(10), { status: 200 }),
    );
    const entry: BuildInfoEntry = {
      path: "/test/data.nos",
      sha1: "",
      file: "data.nos",
      flags: 0,
      size: 100,
      folder: false,
    };
    const data = await downloadEntry(entry);
    expect(fetch).toHaveBeenCalledWith(
      "http://patches.gameforge.com/test/data.nos",
      { headers: { "User-Agent": "GameforgeClient/2.8.5", "Origin": "spark://www.gameforge.com" } },
    );
    expect(data).toBeInstanceOf(Uint8Array);
    expect(data.byteLength).toBe(10);
  });

  it("throws on download failure", async () => {
    vi.spyOn(globalThis, "fetch").mockResolvedValue(new Response(null, { status: 500 }));
    const entry: BuildInfoEntry = {
      path: "/test/data.nos",
      sha1: "",
      file: "data.nos",
      flags: 0,
      size: 100,
      folder: false,
    };
    await expect(downloadEntry(entry)).rejects.toThrow("Failed to download data.nos");
  });
});
