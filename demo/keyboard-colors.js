// Keyboard palette values, shared by keys and group dots.
// Arrays use background, border, bottom border, text, hover background, hover border.
const KEYBOARD_PALETTES = {
  "gold": {
    "dot": "#eab308",
    "light": [
      "#fefce8",
      "#fef08a",
      "#fde047",
      "#854d0e",
      "#fef9c3",
      "#eab308"
    ],
    "dark": [
      "#2b2512",
      "#483d1c",
      "#635424",
      "#fef08a",
      "#383016",
      "#eab308"
    ]
  },
  "slate": {
    "dot": "#94a3b8",
    "light": [
      "#f8fafc",
      "#e2e8f0",
      "#cbd5e1",
      "#334155",
      "#f1f5f9",
      "#94a3b8"
    ],
    "dark": [
      "#1e293b",
      "#334155",
      "#475569",
      "#cbd5e1",
      "#273549",
      "#94a3b8"
    ]
  },
  "purple": {
    "dot": "#a855f7",
    "light": [
      "#faf5ff",
      "#e9d5ff",
      "#d8b4fe",
      "#6b21a8",
      "#f3e8ff",
      "#a855f7"
    ],
    "dark": [
      "#25163a",
      "#432468",
      "#5c328e",
      "#e9d5ff",
      "#321c4e",
      "#a855f7"
    ]
  },
  "rose": {
    "dot": "#f43f5e",
    "light": [
      "#fff1f2",
      "#fecdd3",
      "#fda4af",
      "#9f1239",
      "#ffe4e6",
      "#f43f5e"
    ],
    "dark": [
      "#31141c",
      "#551e2e",
      "#782941",
      "#fecdd3",
      "#401824",
      "#f43f5e"
    ]
  },
  "sky": {
    "dot": "#38bdf8",
    "light": [
      "#f0f9ff",
      "#bae6fd",
      "#7dd3fc",
      "#0369a1",
      "#e0f2fe",
      "#38bdf8"
    ],
    "dark": [
      "#112638",
      "#1e4566",
      "#2b5f8c",
      "#bae6fd",
      "#163249",
      "#38bdf8"
    ]
  },
  "teal": {
    "dot": "#2dd4bf",
    "light": [
      "#f0fdfa",
      "#99f6e4",
      "#5eead4",
      "#0f766e",
      "#ccfbf1",
      "#2dd4bf"
    ],
    "dark": [
      "#0f2824",
      "#1b4942",
      "#26655c",
      "#99f6e4",
      "#153731",
      "#2dd4bf"
    ]
  },
  "emerald": {
    "dot": "#10b981",
    "light": [
      "#ecfdf5",
      "#a7f3d0",
      "#6ee7b7",
      "#047857",
      "#d1fae5",
      "#34d399"
    ],
    "dark": [
      "#0d281e",
      "#1a4f3b",
      "#277557",
      "#a7f3d0",
      "#133a2c",
      "#34d399"
    ]
  },
  "cyan": {
    "dot": "#06b6d4",
    "light": [
      "#ecfeff",
      "#a5f3fc",
      "#67e8f9",
      "#0e7490",
      "#cffafe",
      "#22d3ee"
    ],
    "dark": [
      "#08262c",
      "#154c57",
      "#217282",
      "#a5f3fc",
      "#0e3841",
      "#22d3ee"
    ]
  },
  "amber": {
    "dot": "#eab308",
    "light": [
      "#fefce8",
      "#fde68a",
      "#facc15",
      "#a16207",
      "#fef9c3",
      "#eab308"
    ],
    "dark": [
      "#2b2410",
      "#4d3e16",
      "#715b1f",
      "#fde68a",
      "#3a3116",
      "#eab308"
    ]
  },
  "orange": {
    "dot": "#f97316",
    "light": [
      "#fff7ed",
      "#fed7aa",
      "#fb923c",
      "#c2410c",
      "#ffedd5",
      "#f97316"
    ],
    "dark": [
      "#2d180d",
      "#522b17",
      "#773e21",
      "#fed7aa",
      "#3d2012",
      "#f97316"
    ]
  },
  "coral": {
    "dot": "#f43f5e",
    "light": [
      "#fff1f2",
      "#fecdd3",
      "#fb7185",
      "#be123c",
      "#ffe4e6",
      "#f43f5e"
    ],
    "dark": [
      "#2e1219",
      "#53202d",
      "#792e41",
      "#fecdd3",
      "#3f1922",
      "#f43f5e"
    ]
  },
  "green": {
    "dot": "#4ade80",
    "light": [
      "#f0fdf4",
      "#bbf7d0",
      "#86efac",
      "#15803d",
      "#dcfce7",
      "#4ade80"
    ],
    "dark": [
      "#12281a",
      "#1f472e",
      "#2c6441",
      "#bbf7d0",
      "#183523",
      "#4ade80"
    ]
  },
  "indigo": {
    "dot": "#818cf8",
    "light": [
      "#eef2ff",
      "#c7d2fe",
      "#a5b4fc",
      "#4338ca",
      "#e0e7ff",
      "#818cf8"
    ],
    "dark": [
      "#1c1d38",
      "#2f3263",
      "#41468b",
      "#c7d2fe",
      "#25264b",
      "#818cf8"
    ]
  },
  "peach": {
    "dot": "#fb923c",
    "light": [
      "#fff7ed",
      "#fed7aa",
      "#fdba74",
      "#c2410c",
      "#ffedd5",
      "#fb923c"
    ],
    "dark": [
      "#331d10",
      "#57311a",
      "#7a4524",
      "#fed7aa",
      "#442615",
      "#fb923c"
    ]
  },
  "pink": {
    "dot": "#ec4899",
    "light": [
      "#fdf2f8",
      "#fbcfe8",
      "#f472b6",
      "#be185d",
      "#fce7f3",
      "#ec4899"
    ],
    "dark": [
      "#311525",
      "#552140",
      "#792e5a",
      "#fbcfe8",
      "#421b31",
      "#ec4899"
    ]
  }
};

function installKeyboardColors() {
  const properties = ['background', 'border-color', 'border-bottom-color', 'color', 'hover-background', 'hover-border'];
  const root = [], dark = [], classes = [];
  for (const [name, palette] of Object.entries(KEYBOARD_PALETTES)) {
    root.push(`--${name}-dot:${palette.dot};`);
    properties.forEach((property, index) => {
      root.push(`--${name}-${property}:${palette.light[index]};`);
      dark.push(`--${name}-${property}:${palette.dark[index]};`);
    });
    const variables = ['dot', ...properties].map(property => `--key-${property}:var(--${name}-${property});`).join('');
    classes.push(`.keyboard-palette--${name}{${variables}}`);
  }
  const style = document.createElement('style');
  style.textContent = `:root{${root.join('')}}:root[data-theme="dark"]{${dark.join('')}}${classes.join('')}`;
  document.head.append(style);
}
