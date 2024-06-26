import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import mkcert from 'vite-plugin-mkcert';

// https://vitejs.dev/config/
export default defineConfig(({ mode }) => ({
  base: mode === 'production' ? '/crdbl/' : '/',
  plugins: [react(), mkcert()],
  server: {
    fs: {
      allow: ["../bindings/web/dist", "."]
    }
  }
}));
