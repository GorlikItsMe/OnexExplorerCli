import {EntryInfoRaw, OnexArchiveBinding, OnexModule} from '../onex.js';

const fsStore = new Map<string, Uint8Array>();

class MockArchive implements OnexArchiveBinding {
  private opened_ = false;
  private filepath_ = '';
  private entries_: EntryInfoRaw[] = [];

  delete(): void {
    this.opened_ = false;
    this.entries_ = [];
  }

  open(filepath: string): boolean {
    this.filepath_ = filepath;
    this.opened_ = fsStore.has(filepath);
    if (this.opened_) {
      const data = fsStore.get(filepath)!;
      if (data.length < 16) {
        this.opened_ = false;
        return false;
      }
      this.entries_ = [
        {
          id: 1,
          name: 'item.dat',
          creationDate: 12345,
          compressed: true,
          type: 4,
          offset: 0,
          compressedSize: 100,
          uncompressedSize: 500
        },
        {
          id: 2,
          name: 'icon.png',
          creationDate: 12346,
          compressed: true,
          type: 1,
          offset: 200,
          compressedSize: 2000,
          uncompressedSize: 8000
        },
        {
          id: 3,
          name: 'texture.nos',
          creationDate: 12347,
          compressed: false,
          type: 0,
          offset: 3000,
          compressedSize: 16000,
          uncompressedSize: 16000
        },
      ];
    }
    return this.opened_;
  }

  isOpen(): boolean {
    return this.opened_;
  }
  filepath(): string {
    return this.filepath_;
  }
  entryCount(): number {
    return this.entries_.length;
  }
  lastError(): number {
    return this.opened_ ? 0 : 1;
  }

  readEntry(index: number): Uint8Array|null {
    if (!this.opened_ || index < 0 || index >= this.entries_.length) return null;
    return new Uint8Array([0xDE, 0xAD, 0xBE, 0xEF, 0x42]);
  }

  entryAt(index: number): EntryInfoRaw|null {
    if (!this.opened_ || index < 0 || index >= this.entries_.length) return null;
    return this.entries_[index];
  }

  entries(): EntryInfoRaw[] {
    return this.opened_ ? [...this.entries_] : [];
  }
}

export function createMockModule(): OnexModule {
  return {
    Archive: MockArchive as unknown as new () => OnexArchiveBinding,
    decodeEntryToPng(data: Uint8Array, type: number): Uint8Array | null {
      if (data.length === 0) return null;
      return new Uint8Array([0x89, 0x50, 0x4E, 0x47]); // fake PNG header
    },
    ENTRY_TYPE_TEXTURE: 0,
    ENTRY_TYPE_ICON: 1,
    ENTRY_TYPE_IMAGE4B: 2,
    ENTRY_TYPE_TILE_GRID: 3,
    ENTRY_TYPE_TEXT_DAT: 4,
    ENTRY_TYPE_TEXT_LST: 5,
    ENTRY_TYPE_UNKNOWN: 6,
    FS: {
      writeFile(path: string, data: Uint8Array) { fsStore.set(path, data); },
      unlink(path: string) { fsStore.delete(path); },
    },
  };
}

export function resetFsStore() {
  fsStore.clear();
}
