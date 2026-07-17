import {getModule, OnexArchiveBinding, OnexModule} from './onex.js';
import {EntryInfo, EntryType} from './types.js';

function toEntryInfo(e: {
  id: number; name: string; creationDate: number; compressed: boolean; type: number; offset: number;
  compressedSize: number;
  uncompressedSize: number
}): EntryInfo {
  return {
    id: e.id,
    name: e.name,
    creationDate: e.creationDate,
    compressed: e.compressed,
    type: e.type as EntryType,
    offset: e.offset as number,
    compressedSize: e.compressedSize as number,
    uncompressedSize: e.uncompressedSize as number,
  };
}

export class NosArchive {
  private handle: OnexArchiveBinding|null = null;
  private mod: OnexModule|null = null;

  private safeFilename(filename: string): string {
    return filename || 'archive.nos';
  }

  async open(data: Uint8Array, filename: string = 'archive.nos'): Promise<void> {
    this.close();
    this.mod = await getModule();
    this.filepath_ = '/' + this.safeFilename(filename);
    this.mod.FS.writeFile(this.filepath_, data);
    const handle = new this.mod.Archive();
    const ok = handle.open(this.filepath_);
    if (!ok) {
      handle.delete();
      this.mod.FS.unlink(this.filepath_);
      this.mod = null;
      this.filepath_ = '';
      throw new Error(`Failed to open archive (error code: ${handle.lastError()})`);
    }
    this.handle = handle;
  }

  get entryCount(): number {
    if (!this.handle) return 0;
    return this.handle.entryCount();
  }

  get isOpen(): boolean {
    return this.handle !== null && this.handle.isOpen();
  }

  getEntries(): EntryInfo[] {
    if (!this.handle) return [];
    return this.handle.entries().map(toEntryInfo);
  }

  getEntry(index: number): EntryInfo|null {
    if (!this.handle) return null;
    const e = this.handle.entryAt(index);
    if (!e) return null;
    return toEntryInfo(e);
  }

  readEntry(index: number): Uint8Array|null {
    if (!this.handle) return null;
    const data = this.handle.readEntry(index);
    return data.length > 0 ? data : null;
  }

  readEntryAsPng(index: number): Uint8Array|null {
    if (!this.mod || !this.handle) return null;
    const data = this.handle.readEntry(index);
    if (data.length === 0) return null;
    const entry = this.handle.entryAt(index);
    if (!entry) return null;
    const png = this.mod.decodeEntryToPng(data, entry.type);
    return png.length > 0 ? png : null;
  }

  private filepath_: string = '';

  close(): void {
    if (this.handle) {
      this.handle.delete();
    }
    if (this.mod && this.filepath_) {
      try {
        this.mod.FS.unlink(this.filepath_);
      } catch {
      }
    }
    this.handle = null;
    this.mod = null;
    this.filepath_ = '';
  }
}
