import react from '@vitejs/plugin-react'
import { defineConfig } from 'vite'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    watch: {
      usePolling: true,
      interval: 100, // checks for changes every 100ms
      ignored: [
        '**/node_modules/**',
        '**/.git/**',
        '**/.pio/**',
        '**/server/**',
        '**/dist/**'
      ]
    }
  }
})
