import { spawn, type ChildProcessWithoutNullStreams } from 'node:child_process';
import { EventEmitter } from 'node:events';
import { createInterface } from 'node:readline';
import type { Position } from '../shared/types.js';

export class EngineClient extends EventEmitter {
  private child: ChildProcessWithoutNullStreams;
  private sequence = 0;
  private waiting = new Map<number, { resolve: (value: any) => void; reject: (reason: Error) => void }>();
  private alive = true;
  private ending = false;
  readonly ready: Promise<Position>;

  constructor(executable: string, cwd: string) {
    super();
    this.child = spawn(executable, [], { cwd, windowsHide: true, stdio: 'pipe' });
    this.ready = new Promise((resolve, reject) => {
      const timer = setTimeout(() => { reject(new Error('The engine did not start within 30 seconds.')); this.child.kill(); }, 30_000);
      this.once('ready', (position: Position) => { clearTimeout(timer); resolve(position); });
      this.once('failure', (error: Error) => { clearTimeout(timer); reject(error); });
    });
    const lines = createInterface({ input: this.child.stdout });
    lines.on('line', line => {
      try {
        const message = JSON.parse(line);
        if (message.event === 'ready') this.emit('ready', message.data);
        else if (message.event === 'fatal') this.fail(new Error(message.error));
        else if (message.event === 'progress') this.emit('progress', message);
        else if (typeof message.id === 'number') {
          const pending = this.waiting.get(message.id);
          if (!pending) return;
          this.waiting.delete(message.id);
          if (message.ok) pending.resolve(message.data);
          else pending.reject(new Error(message.error || 'The engine rejected the command.'));
        } else this.emit('log', line + '\n');
      } catch { this.emit('log', line + '\n'); }
    });
    this.child.stderr.setEncoding('utf8');
    this.child.stderr.on('data', text => this.emit('log', text));
    this.child.on('error', error => this.fail(error));
    this.child.stdin.on('error', error => this.fail(error));
    this.child.on('exit', (code, signal) => {
      this.alive = false;
      const error = new Error(`Engine exited (${signal ?? code}). Restart the app to reconnect.`);
      if (!this.ending) this.fail(error);
      else for (const pending of this.waiting.values()) pending.reject(error);
      this.waiting.clear();
      this.emit('closed');
    });
  }

  private fail(error: Error) {
    if (this.ending) return;
    this.alive = false;
    for (const pending of this.waiting.values()) pending.reject(error);
    this.waiting.clear();
    this.emit('failure', error);
  }

  request<T = any>(command: string, arguments_: Record<string, unknown> = {}): Promise<T> {
    if (!this.alive) return Promise.reject(new Error('The engine is not connected.'));
    const id = ++this.sequence;
    return new Promise((resolve, reject) => {
      this.waiting.set(id, { resolve, reject });
      this.child.stdin.write(JSON.stringify({ ...arguments_, command, id }) + '\n');
    });
  }

  async close() {
    if (this.ending) return;
    this.ending = true;
    if (!this.alive) { this.child.kill(); return; }
    const deadline = setTimeout(() => this.child.kill(), 4000);
    try { await this.request('quit'); this.child.stdin.end(); } catch { this.child.kill(); }
    if (this.alive) await new Promise<void>(resolve => this.once('closed', resolve));
    clearTimeout(deadline);
  }
}
