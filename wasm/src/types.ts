export enum EntryType {
  Texture = 0,
  Icon = 1,
  Image4B = 2,
  TileGrid = 3,
  TextDat = 4,
  TextLst = 5,
  Unknown = 6,
}

export interface EntryInfo {
  id: number;
  name: string;
  creationDate: number;
  compressed: boolean;
  type: EntryType;
  offset: number;
  compressedSize: number;
  uncompressedSize: number;
}

export interface ArchiveOpenOptions {
  filename?: string;
}
