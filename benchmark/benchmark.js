/**
 * @typedef {Object} ProfileResult
 * @property {number} count
 * @property {number} totalTime
 * @property {number} minTime
 * @property {number} maxTime
 */

/**
 * @typedef {Object.<string, ProfileResult>} ProfileResults
 */

/**
 * @type {ProfileResults}
 */
let profileResults = {};
/**
 * @type {ProfileResults[]}
 */
let benchmarkResults = [];

/**
 * @param {Function} fn
 * @param {number} numRuns
 * @returns {ProfileResults[]}
 */
export const benchmark = (fn, numRuns) => {
  profileResults = {};
  benchmarkResults = [];

  for (let i = 0; i < numRuns; i++) {
    console.log("Run", i + 1, "of", numRuns);

    profile("Total", fn);

    benchmarkResults.push(profileResults);
    profileResults = {};

    const currentAvg = benchmarkResults.reduce(
      (acc, profileResult) => acc + profileResult["Total"].totalTime, 0) / benchmarkResults.length;
    console.log(`  Last: ${benchmarkResults[benchmarkResults.length - 1]["Total"].totalTime}ms`);
    console.log(`  Avg: ${currentAvg}ms`);
    console.log(`  ETA: ${(numRuns - i - 1) * currentAvg * 0.001}s`);
  }

  const results = benchmarkResults;
  benchmarkResults = [];
  return results;
};

/**
 * @param {string} name
 * @param {Function} fn
 */
export const profile = (name, fn) => {
  const start = performance.now();
  const result = fn();
  const end = performance.now();
  const time = end - start;

  if (!(name in profileResults)) {
    profileResults[name] = {
      count: 0,
      totalTime: 0,
      minTime: Number.MAX_VALUE,
      maxTime: Number.MIN_VALUE
    };
  }
  const profileResult = profileResults[name];
  profileResult.count++;
  profileResult.totalTime += time;
  profileResult.minTime = Math.min(profileResult.minTime, time);
  profileResult.maxTime = Math.max(profileResult.maxTime, time);

  return result;
};

/**
 * @param {ProfileResults[]} results
 */
export const printResults = (results) => {
  if (!results.length)
    return;

  const keys = Object.keys(results[0]);

  console.log("Summary");

  // for each profile key, print the average and a confidence interval
  const padSize = keys.reduce((acc, key) => Math.max(acc, key.length), 0) + 2;
  for (const key of keys) {
    // const times = results.map((profileResult) => profileResult[key].totalTime);
    const times = results.map((profileResult) => profileResult[key].totalTime / profileResult[key].count);
    const avg = times.reduce((acc, time) => acc + time, 0) / times.length;
    const variance = times.reduce((acc, time) => acc + (time - avg) ** 2, 0) / times.length;
    const stdDev = Math.sqrt(variance);
    const confidence = 1.96 * stdDev / Math.sqrt(times.length);
    const totalSamples = results.reduce((acc, profileResult) => acc + profileResult[key].count, 0);

    console.log(`${key}:`.padEnd(padSize), `${avg.toFixed(3)}ms ± ${confidence.toFixed(3)}ms (${totalSamples} samples)`);
  }
}