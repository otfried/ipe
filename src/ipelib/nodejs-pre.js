//
// provide environment to code running under NodeJS
//

const { env } =  require('node:process');

var Module = {
  'preRun': () => {
    ENV = {...env, ...ENV};
  },
};
