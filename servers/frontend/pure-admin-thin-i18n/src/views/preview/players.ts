import type { InjectionKey } from "vue";

import { WhepPlayer } from "./whep";

export class StreamPlayers {
  players: Map<string, WhepPlayer>;

  constructor() {
    this.players = new Map();
  }

  attach(host: HTMLElement, id: string, url: string) {
    const player = this.players.get(id) ?? new WhepPlayer();
    this.players.set(id, player);
    host.replaceChildren(player.open(url));
  }

  detach(host: HTMLElement) {
    host.replaceChildren();
  }

  retain(ids: string[]) {
    const live = new Set(ids);
    for (const [id, player] of [...this.players]) {
      if (!live.has(id)) {
        player.stop();
        this.players.delete(id);
      }
    }
  }
}

export const streamPlayersKey: InjectionKey<StreamPlayers> = Symbol("streamPlayers");

export const requireStreamPlayers = (players: StreamPlayers | undefined) => {
  if (!players) {
    throw new Error("StreamPlayers missing");
  }
  return players;
};
