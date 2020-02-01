const request = require('request-promise');

const OPCODES = Object.freeze({
  JMP: '0x01',
  AIC: '0x02',
  L2M: '0x07',
  ECH: '0xe1',
  ADMP: '0xe2',
  CDMP: '0xe3',
  DECOUT: '0xf5'
});

const memoryDataRaw = require('./station.hex.json');

const protocol = 'http';
const host = '192.168.7.68';

const startCommands = [
  { url: `${protocol}://${host}/exec?command=${OPCODES.JMP}&p1=0`, comment: 'Setting ADDR to 0' },
  { url: `${protocol}://${host}/exec?command=${OPCODES.ECH}&p1=1`, comment: 'Setting Echo to 1' },
  { url: `${protocol}://${host}/exec?command=${OPCODES.DECOUT}&p1=1`, comment: 'Setting DECOUT to 1' }
];

const canStart = new Promise((resolveStart) => {
  startCommands.reduce(function(p, item) {
    return p.then(() => {
        console.log(item.comment);
        return request(item.url).then((res) => {
          console.log(res);
        });
    });
  }, Promise.resolve()).then(function() {
    resolveStart();
  }).catch(function(err) {
      console.log('Start Commands Error:', err);
      process.exit(1);
  });
});

const requestsList = [];

let memoryData = [];

if (typeof(memoryDataRaw.hex) === 'object') {
  memoryData = Object.values(memoryDataRaw.hex);
} else {
  memoryData = memoryDataRaw.hex;
}

for (let i = 0; i < memoryData.length; i++) {
  requestsList.push({
    url: `${protocol}://${host}/exec?command=${OPCODES.L2M}&p1=${memoryData[i]}`,
    comment: `Memory Location: ${i}, value: ${memoryData[i].toString(16)}, Char: ${String.fromCharCode(memoryData[i])}`
  });
  requestsList.push({
    url: `${protocol}://${host}/exec?command=${OPCODES.AIC}`
  });
}

canStart.then(() => {
  console.log('Events to dispatch: ', requestsList.length);
  requestsList.reduce(function(p, item) {
    return p.then(() => {
      if (item.comment) {
        console.log(item.comment);
      }
      return request(item.url).then((res) => {
        console.log('Reply: ', res);
      });
    });
  }, Promise.resolve()).then(function() {
    console.log('Completed');
  }).catch(function(err) {
      console.log('Request Loop Error:', err);
  });
});
