import { resolve } from 'node:path'
import { defineConfig, externalizeDepsPlugin } from 'electron-vite'

export default defineConfig({
  main: {
    plugins: [externalizeDepsPlugin()]
  },
  preload: {
    plugins: [externalizeDepsPlugin()]
  },
  renderer: {
    server: {
      fs: {
        // Allow serving files from the sibling ipe-web folder
        allow: [
          resolve(__dirname, '.'),
          resolve(__dirname, '../ipe-web')
        ]
      }
    }
  }
})
