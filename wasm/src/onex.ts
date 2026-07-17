export interface OnexModule {
  Archive: new() => OnexArchiveBinding;
  decodeEntryToPng(data: Uint8Array, type: number): Uint8Array;
  ENTRY_TYPE_TEXTURE: number;
  ENTRY_TYPE_ICON: number;
  ENTRY_TYPE_IMAGE4B: number;
  ENTRY_TYPE_TILE_GRID: number;
  ENTRY_TYPE_TEXT_DAT: number;
  ENTRY_TYPE_TEXT_LST: number;
  ENTRY_TYPE_UNKNOWN: number;
  FS: {writeFile(path: string, data: Uint8Array): void; unlink(path: string): void;};
}

export interface OnexArchiveBinding {
  open(filepath: string): boolean;
  isOpen(): boolean;
  filepath(): string;
  entryCount(): number;
  readEntry(index: number): Uint8Array;
  entryAt(index: number): EntryInfoRaw|null;
  entries(): EntryInfoRaw[];
  lastError(): number;
  delete(): void;
}

export interface EntryInfoRaw {
  id: number;
  name: string;
  creationDate: number;
  compressed: boolean;
  type: number;
  offset: number;
  compressedSize: number;
  uncompressedSize: number;
}

type ModuleFactory = () => Promise<OnexModule>;

let modulePromise: Promise<OnexModule>|null = null;

export async function getModule(): Promise<OnexModule> {
  if (modulePromise) return modulePromise;

  modulePromise = (async () => {
    const createModule
        = (await import('../wasm/onex_explorer_wasm.js')) as unknown as ModuleFactory;
    const mod = await createModule();
    return mod as unknown as OnexModule;
  })();

  return modulePromise;
}

export function resetModule(): void {
  modulePromise = null;
}
