type EventEmitterWatcher<Args extends unknown[]> = {
  callback: (...args: Args) => void;
  bindTo?: unknown;
}

export default class EventEmitter<Args extends unknown[]> {
  private readonly watchers: Array<EventEmitterWatcher<Args>>;
  private isUpdating: boolean;
  private updateIndex: number;

  constructor() {
    this.watchers = [];
    this.isUpdating = false;
    this.updateIndex = 0;
  }

  has(callback: (...args: Args) => void, bindTo?: unknown) {
    return this.watchers.some(watcher =>
      watcher.callback === callback && watcher.bindTo === bindTo);
  }

  watch(callback: (...args: Args) => void, bindTo?: unknown) {
    this.watchers.push({ callback, bindTo });
  }

  unwatch(callback: (...args: Args) => void, bindTo?: unknown) {
    const idx = this.watchers.findIndex(watcher =>
      watcher.callback === callback && watcher.bindTo === bindTo);
    if (idx < 0) return;

    this.watchers.splice(idx, 1);
    if (this.isUpdating && idx <= this.updateIndex) {
      this.updateIndex--;
    }
  }

  emit(...values: Args) {
    this.isUpdating = true;
    for (this.updateIndex = 0; this.updateIndex < this.watchers.length; this.updateIndex++) {
      const watcher = this.watchers[this.updateIndex];
      if (watcher.bindTo) {
        const bindTo = watcher.bindTo as ThisType<unknown>;
        watcher.callback.apply(bindTo, values);
      } else {
        watcher.callback(...values);
      }
    }
    this.isUpdating = false;
  }
}
