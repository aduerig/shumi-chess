import { contextBridge, ipcRenderer } from 'electron';

// This is the only desktop-specific interface used by the renderer. A later web
// client can provide the same methods with HTTP/WebSocket transport.
contextBridge.exposeInMainWorld('shumi', {
  state: () => ipcRenderer.invoke('shumi:state'),
  dispatch: (action: unknown) => ipcRenderer.invoke('shumi:action', action),
  copy: (kind: string) => ipcRenderer.invoke('shumi:copy', kind),
  quit: () => ipcRenderer.invoke('shumi:quit'),
  subscribe: (callback: (event: unknown) => void) => {
    const handler = (_event: Electron.IpcRendererEvent, message: unknown) => callback(message);
    ipcRenderer.on('shumi:event', handler);
    return () => ipcRenderer.removeListener('shumi:event', handler);
  },
});
