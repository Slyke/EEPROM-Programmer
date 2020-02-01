const memoryDataRaw = require('./station.hex.json');

let memoryData = [];

if (typeof(memoryDataRaw.hex) === 'object') {
  memoryData = Object.values(memoryDataRaw.hex);
} else {
  memoryData = memoryDataRaw.hex;
}

for (let i = 0; i < memoryData.length; i++) {
    process.stdout.write(String.fromCharCode(memoryData[i]));
}
  
