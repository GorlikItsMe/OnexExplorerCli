export interface BuildInfoEntry {
  path: string;
  sha1: string;
  file: string;
  flags: number;
  size: number;
  folder: boolean;
}

export interface BuildInfo {
  entries: BuildInfoEntry[];
  totalSize: number;
  build: number;
}

const defaultHeaders: Record<string, string> = {
  'User-Agent': 'GameforgeClient/2.8.5',
  'Origin': 'spark://www.gameforge.com',
};

export async function fetchBuildInfo(
    gameId: string,
    buildId: string,
    baseUrl?: string,
    ): Promise<BuildInfo> {
  const cdn = baseUrl ?? 'https://spark.gameforge.com';
  const url = buildId === 'latest'
      ? `${cdn}/api/v1/patching/download/latest/${gameId}/default`
      : `${cdn}/api/v1/patching/download/install/${gameId}/default/${buildId}`;
  const response = await fetch(url, {headers: defaultHeaders});
  if (!response.ok) {
    throw new Error(`Failed to fetch build info: ${response.statusText}`);
  }
  const json = await response.json();
  const entries: BuildInfoEntry[] = (json.entries ?? []).map((e: any) => ({
                                                               path: e.path ?? '',
                                                               sha1: e.sha1 ?? '',
                                                               file: e.file ?? '',
                                                               flags: e.flags ?? 0,
                                                               size: e.size ?? 0,
                                                               folder: e.folder ?? false,
                                                             }));
  return {
    entries,
    totalSize: json.totalSize ?? 0,
    build: json.build ?? 0,
  };
}

export function makeDownloadUrl(entry: BuildInfoEntry): string {
  return 'http://patches.gameforge.com' + entry.path;
}

export function resolve(entries: BuildInfoEntry[], name: string): BuildInfoEntry|null {
  const exact = entries.filter(e => !e.folder && e.file !== '' && e.file === name);
  if (exact.length === 1) return exact[0];
  if (exact.length > 1) return null;

  const bare = entries.filter(e => {
    if (e.folder || e.file === '') return false;
    const filename = e.file.replace(/\\/g, '/').split('/').pop() ?? '';
    return filename === name;
  });
  if (bare.length === 1) return bare[0];
  if (bare.length > 1) return null;

  return null;
}

export async function downloadEntry(entry: BuildInfoEntry): Promise<Uint8Array> {
  const url = makeDownloadUrl(entry);
  const response = await fetch(url, {headers: defaultHeaders});
  if (!response.ok) {
    throw new Error(`Failed to download ${entry.file}: ${response.statusText}`);
  }
  return new Uint8Array(await response.arrayBuffer());
}
