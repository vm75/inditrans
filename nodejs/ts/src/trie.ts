type TrieNode<ValueType> = Record<number, { value?: ValueType, nodes?: TrieNode<ValueType> }>;

export class Trie<ValueType> {
  root: TrieNode<ValueType> = {};

  addLookups(keys: string[], values: ValueType[]) {
    keys.forEach((key, idx) => {
      this.addLookup(key, values[idx]);
    })
  }

  addLookup(key: string, value: ValueType) {
    let lookupNode = this.root;
    for (let index = 0; index < key.length; index++) {
      const nextChar = key.charCodeAt(index);
      let lookup = lookupNode[nextChar];
      if (lookup === undefined) {
        lookup = {};
        lookupNode[nextChar] = lookup;
      }

      if (index === key.length - 1) {
        // last lookupNode is the end of a word
        if (lookup.value) {
          if (JSON.stringify(lookup.value) !== JSON.stringify(value)) {
            return lookup.value;
          }
          continue;
        }
        lookup.value = value;
      } else {
        if (!lookup.nodes) {
          lookup.nodes = {};
        }
        lookupNode = lookup.nodes;
      }
    }
  }

  lookup(text: string, startIndex: number): { match: ValueType | undefined; len: number; } {
    let lastMatch;
    let lastMatchIndex = 0;
    let index = startIndex;
    let lookupNode = this.root;
    while (index < text.length) {
      const lookup = lookupNode[text.charCodeAt(index++)];
      if (lookup) {
        if (lookup.value) {
          lastMatch = lookup.value;
          lastMatchIndex = index;
        }

        if (lookup.nodes) {
          lookupNode = lookup.nodes;
          continue;
        }
      }
      break;
    }

    return { match: lastMatch, len: lastMatchIndex - startIndex };
  }
}
