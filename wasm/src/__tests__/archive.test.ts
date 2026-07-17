import { describe, it, expect, beforeEach, vi } from "vitest";
import { NosArchive } from "../archive.js";
import { EntryType } from "../types.js";
import { createMockModule, resetFsStore } from "./mock.js";

vi.mock("../onex.js", () => ({
  getModule: vi.fn(async () => createMockModule()),
  resetModule: vi.fn(),
}));

beforeEach(() => {
  resetFsStore();
  vi.clearAllMocks();
});

const fakeNos = new Uint8Array(256);

describe("NosArchive", () => {
  it("opens from bytes and reports entries", async () => {
    const archive = new NosArchive();
    await archive.open(fakeNos, "test.nos");
    expect(archive.entryCount).toBe(3);
  });

  it("throws on open failure", async () => {
    const archive = new NosArchive();
    await expect(archive.open(new Uint8Array(0), "")).rejects.toThrow("Failed to open archive");
  });

  it("returns mapped entries", async () => {
    const archive = new NosArchive();
    await archive.open(fakeNos);
    const entries = archive.getEntries();
    expect(entries).toHaveLength(3);
    expect(entries[0]).toMatchObject({ id: 1, name: "item.dat", type: EntryType.TextDat });
    expect(entries[1]).toMatchObject({ id: 2, name: "icon.png", type: EntryType.Icon });
    expect(entries[2]).toMatchObject({ id: 3, name: "texture.nos", type: EntryType.Texture });
  });

  it("returns entry by index", async () => {
    const archive = new NosArchive();
    await archive.open(fakeNos);
    const entry = archive.getEntry(1);
    expect(entry).not.toBeNull();
    expect(entry!.name).toBe("icon.png");
  });

  it("returns null for out-of-range entry", async () => {
    const archive = new NosArchive();
    await archive.open(fakeNos);
    expect(archive.getEntry(99)).toBeNull();
  });

  it("reads entry body", async () => {
    const archive = new NosArchive();
    await archive.open(fakeNos);
    const data = archive.readEntry(0);
    expect(data).toBeInstanceOf(Uint8Array);
    expect(data!.length).toBe(5);
  });

  it("reads entry as PNG via the decode pipeline", async () => {
    const archive = new NosArchive();
    await archive.open(fakeNos);
    const png = archive.readEntryAsPng(1);
    expect(png).toBeInstanceOf(Uint8Array);
    expect(png![0]).toBe(0x89);
    expect(png![1]).toBe(0x50); // P
    expect(png![2]).toBe(0x4E); // N
    expect(png![3]).toBe(0x47); // G
  });

  it("returns null from readEntry when closed", () => {
    const archive = new NosArchive();
    expect(archive.readEntry(0)).toBeNull();
    expect(archive.readEntryAsPng(0)).toBeNull();
    expect(archive.getEntries()).toEqual([]);
  });
});
