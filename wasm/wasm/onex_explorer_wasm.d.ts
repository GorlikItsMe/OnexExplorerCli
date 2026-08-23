export interface OnexExplorerWasmModule {
  Archive: new() => {
    open(filepath: string): boolean;
    isOpen(): boolean;
    filepath(): string;
    entryCount(): number;
    readEntry(index: number): Uint8Array|null;
    entryAt(index: number): {
      id: number; name: string; creationDate: number; compressed: boolean; type: number;
      offset: number;
      compressedSize: number;
      uncompressedSize: number;
    }|null;
    entries(): Array<{
      id: number; name: string; creationDate: number; compressed: boolean; type: number;
      offset: number;
      compressedSize: number;
      uncompressedSize: number;
    }>;
    lastError(): number;
  };
  decodeEntryToPng(data: Uint8Array, type: number): Uint8Array|null;
  ENTRY_TYPE_TEXTURE: number;
  ENTRY_TYPE_ICON: number;
  ENTRY_TYPE_IMAGE4B: number;
  ENTRY_TYPE_TILE_GRID: number;
  ENTRY_TYPE_TEXT_DAT: number;
  ENTRY_TYPE_TEXT_LST: number;
  ENTRY_TYPE_UNKNOWN: number;
  FS: {writeFile(path: string, data: Uint8Array): void; unlink(path: string): void;};
}

declare const createModule: () => Promise<OnexExplorerWasmModule>;
export default createModule;
