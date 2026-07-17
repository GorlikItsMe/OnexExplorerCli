import {getModule, OnexArchiveBinding, OnexModule} from './onex.js';
import {EntryInfo, EntryType} from './types.js';

export class NosArchive {
  private handle: OnexArchiveBinding|null = null;
  private mod: OnexModule|null = null;

  async open(data: Uint8Array, filename: string = 'archive.nos'): Promise<void> {
    this.mod = await getModule();
    this.filepath_ = '/' + filename;
    this.mod.FS.writeFile(this.filepath_, data);
    const handle = new this.mod.Archive();
    const ok = handle.open(this.filepath_);
    if (!ok) {
      this.close();
      throw new Error(`Failed to open archive (error code: ${handle.lastError()})`);
    }
    this.handle = handle;
  }

  get entryCount(): number {
    if (!this.handle) return 0;
    return this.handle.entryCount();
  }

  getEntries(): EntryInfo[] {
    if (!this.handle) return [];
    return this.handle.entries().map((e) => ({
                                       id: e.id,
                                       name: e.name,
                                       creationDate: e.creationDate,
                                       compressed: e.compressed,
                                       type: e.type as EntryType,
                                       offset: e.offset as number,
                                       compressedSize: e.compressedSize as number,
                                       uncompressedSize: e.uncompressedSize as number,
                                     }));
  }

  getEntry(index: number): EntryInfo|null {
    if (!this.handle) return null;
    const e = this.handle.entryAt(index);
    if (!e) return null;
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

  readEntry(index: number): Uint8Array|null {
    if (!this.handle) return null;
    return this.handle.readEntry(index);
  }

  readEntryAsPng(index: number): Uint8Array|null {
    if (!this.mod || !this.handle) return null;
    const data = this.handle.readEntry(index);
    if (!data) return null;
    const entry = this.handle.entryAt(index);
    if (!entry) return null;
    return this.mod.decodeEntryToPng(data, entry.type);
  }

  private filepath_: string = '';

  close(): void {
    if (this.handle) {
      (this.handle as any).delete();
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
