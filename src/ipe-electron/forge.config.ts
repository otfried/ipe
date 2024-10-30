import type { ForgeConfig } from '@electron-forge/shared-types';
import { FusesPlugin } from '@electron-forge/plugin-fuses';
import { FuseV1Options, FuseVersion } from '@electron/fuses';
import { MakerDeb } from '@electron-forge/maker-deb';
import { MakerRpm } from '@electron-forge/maker-rpm';

const config: ForgeConfig = {
  packagerConfig: {
    ignore: [
      /^\/src/,
      /^\/(biome|electron|forge|tsconfig)/,
      /\.(ipe|pdf)$/,
    ],
    asar: true,
  },
  rebuildConfig: {},
  makers: [
    new MakerDeb({
      options: {
        maintainer: 'Otfried Cheong',
        homepage: 'https://ipe.otfried.org',
      },
    }, ['linux']),
    new MakerRpm({
      options: {
        homepage: 'https://ipe.otfried.org',
      },
    }, ['linux']),
  ],
  plugins: [
    // Fuses are used to enable/disable various Electron functionality
    // at package time, before code signing the application
    new FusesPlugin({
      version: FuseVersion.V1,
      [FuseV1Options.RunAsNode]: false,
      [FuseV1Options.EnableCookieEncryption]: true,
      [FuseV1Options.EnableNodeOptionsEnvironmentVariable]: false,
      [FuseV1Options.EnableNodeCliInspectArguments]: false,
      [FuseV1Options.EnableEmbeddedAsarIntegrityValidation]: true,
      [FuseV1Options.OnlyLoadAppFromAsar]: true,
    }),
  ],
};

export default config;
