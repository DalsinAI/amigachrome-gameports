'use strict';

/* Download one public zero-cost itch.io upload. The underlying library
 * refuses paid/private pages and does not bypass access controls. */

const path = require('path');
const { downloadGame } = require('itchio-downloader');

async function main() {
  const [url, output, desiredName] = process.argv.slice(2);
  if (!url || !output || !desiredName) {
    throw new Error('usage: itch_download_one.js URL OUTPUT DESIRED_NAME');
  }

  const result = await downloadGame({
    itchGameUrl: url,
    downloadDirectory: path.resolve(output),
    desiredFileName: desiredName,
    platform: 'amiga',
    writeMetaData: true,
    retries: 2,
    retryDelayMs: 750,
    resume: true,
  });

  const safe = { ...result };
  delete safe.fileBuffer;
  console.log('ITCH_RESULT_JSON=' + JSON.stringify(safe));
  process.exitCode = result.status ? 0 : 2;
}

main().catch((error) => {
  console.log('ITCH_RESULT_JSON=' + JSON.stringify({
    status: false,
    failReason: 'exception',
    message: error && error.message ? error.message : String(error),
  }));
  process.exitCode = 1;
});
