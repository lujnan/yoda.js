'use strict'

var logger = require('logger')('main')
var exodus = require('@yoda/exodus')
var os = require('os')

var systemConfig = {}
try {
  systemConfig = require('/etc/yoda/system-config.json')
} catch (error) {
  systemConfig = {}
  logger.info('Not found: system-config.json')
}

// optimized startup
if (systemConfig && systemConfig.bootPriority) {
  os.setPriority(systemConfig.bootPriority)
}

require('@yoda/oh-my-little-pony')
  .catchUncaughtError('/data/system/yodart-err.log')
  .healthReport('vuid')

var AppRuntime = require('../../lib/app-runtime')

;(function init () {
  activateProcess()
  entry()
})()

function activateProcess () {
  // currently this is a workaround for nextTick missing.
  setInterval(() => false, 1000)
}

function entry () {
  logger.info('trying to migrate from lua')
  exodus((err) => {
    if (err) {
      logger.error('Unexpected error on migration from lua', err.stack)
    }

    logger.debug('vui is started')

    var runtime = new AppRuntime()
    runtime.on('on-ready', () => {
      // reset bootPriority
      os.setPriority(0)
    })
    runtime.init()

    // starts the watchdog when the runtime is initialized.
    require('./watchdog').startFeeding((err) => {
      if (err) {
        logger.error(`watchdog failed to create(${err && err.message}), just exits`)
        process.exit(1)
      }
    })
  })
}
