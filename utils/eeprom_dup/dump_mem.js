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

const getUntilAddress = 0x420;
const getAtATime = 20;
const protocol = 'http';
const host = '192.168.7.68';

const bufferBreach = [];

const eepromData = new Uint8Array(getUntilAddress);

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
const maxRequestsReq = Math.ceil(getUntilAddress / (getAtATime - 1)) - 1;

for (let i = 0; i < maxRequestsReq; i++) {
  requestsList.push(`${protocol}://${host}/exec?command=${OPCODES.CDMP}&p1=${getAtATime}`);
}

const resultToArr = (result, pcIndex) => {
  const confirmRes = result.split("\n");
  const resultArr = confirmRes[0].trim().split(' ');

  for (let i = 0; i < resultArr.length; i++) {
    if (pcIndex + i < getUntilAddress) {
      eepromData[pcIndex + i] = resultArr[i];
    } else {
      bufferBreach.push(pcIndex, i);
    }
  }

  return [resultArr.length, pcIndex];
};

let pcAddress = 0;

canStart.then(() => {
  console.log('Events to dispatch: ', requestsList.length);
  requestsList.reduce(function(p, item) {
    return p.then(() => {
        const endAddress = pcAddress + getAtATime;
        console.log(`Getting:  Addresses: ${pcAddress}-${endAddress - 1} - ${item}`);
        return request(item).then((res) => {
          const mergeResult = resultToArr(res, pcAddress);
          pcAddress = (Math.abs(1 - getAtATime) + pcAddress);
        });
    });
  }, Promise.resolve()).then(function() {
    console.log('Data: ', JSON.stringify(Array.from(eepromData), null, 2));
    console.log('Indexed Data: ', JSON.stringify(eepromData, null, 2));

    if (bufferBreach.length > 0) {
      console.log('Buffer Breach: ');
      console.log(JSON.stringify(bufferBreach, null, 2));
    }
  }).catch(function(err) {
      console.log('Request Loop Error:', err);
  });
});
