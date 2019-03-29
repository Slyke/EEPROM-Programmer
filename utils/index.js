let opCommand = "0x06";
let processArgs = process.argv;
let charComment = false;
let stringTerminatorChar = true;
let assemlyLineLength = 2; // Newlines every X commands, to avoid serial overlfow.
let assemblyOutput = false;
let argCount = 0;
let padding = 2;

let assemblyOut = "";
let commandCounter = 0;
let terminatorLoc = 0;

for (let i = 2; i < processArgs.length; i++) {
  if (processArgs[i] === "-op") {
    if (processArgs[i + 1] !== undefined) {
      opCommand = processArgs[i + 1];
      argCount += 2;
    }
  } else if (processArgs[i] === "-c") {
    charComment = true;
    argCount++;
  } else if (processArgs[i] === "-nt") {
    stringTerminatorChar = false;
    argCount++;
  } else if (processArgs[i] === "-asm") {
    assemblyOutput = true;
    argCount++;
  } else if (processArgs[i] === "-p") {
    if (processArgs[i + 1] !== undefined) {
      padding = parseInt(processArgs[i + 1]);
    }
    
    argCount += 2;
  } else if (processArgs[i] === "-asml") {
    if (processArgs[i + 1] !== undefined) {
      assemlyLineLength = parseInt(processArgs[i + 1]);
    }
    
    argCount += 2;
  }
}

if (processArgs.length < argCount + 3) {
  console.error("Not enough arguments.");
  process.exit(1);
}

function pad(str, size, withChar = "0", atEnd = false) {
  var s = str + "";
  if (atEnd) {
    while (s.length < size) {
      s = s + withChar;
    }
  } else {
    while (s.length < size) {
      s = withChar + s;
    }
  }
  return s;
}

function convertToHex(inputChar, charIndex) {
  if (inputChar === 0x00) {
    if (charComment) {
      return `0x${pad('00', padding > -1 ? padding : 2)} // Input (${charIndex}): \\0`;
    }
    return `0x${pad('00', padding > -1 ? padding : 2)}`;
  }

  if (charComment) {
    return `0x${pad(inputChar.charCodeAt(0).toString(16), padding > -1 ? padding : 2)} // Input (${charIndex}): ${inputChar[0]}`;
  }
  return "0x" + pad(inputChar.charCodeAt(0).toString(16), padding > -1 ? padding : 2);
}

for (let i = argCount + 2; i < processArgs.length; i += 2) {
  if (processArgs[i] === "-op" ||  processArgs[i] === "-asml" ||  processArgs[i] === "-p") {
    continue;
  }

  if (processArgs[i] === "-c" || processArgs[i] === "-nt" || processArgs[i] === "-asm") {
    i--;
    continue;
  }
  if (processArgs[i + 1] === undefined) {
    console.error("No string arguement at param", i);
    process.exit(2);
  }
  
  let startingAddress = 0;
  try {
    startingAddress = parseInt(processArgs[i]);
  } catch (err) {
    console.error(err);
  }

  let processString = processArgs[i + 1];

  if (assemblyOutput) {
    for (let j = 0; j < processString.length; j++) {
      assemblyOut += `${opCommand} 0x${pad((startingAddress + j).toString(16), padding > -1 ? padding : 4)} ${convertToHex(processString[j], j + 1)} `;
      commandCounter++;
      if (assemlyLineLength > 0 && commandCounter > assemlyLineLength) {
        console.log(assemblyOut);
        commandCounter = 0;
        assemblyOut = "";
      }
      terminatorLoc = startingAddress + j + 1;
    }
    if (assemblyOut === "") {
      if (stringTerminatorChar) {
        assemblyOut += `${opCommand} 0x${pad((terminatorLoc).toString(16), padding > -1 ? padding : 4)} ${convertToHex(0x00, terminatorLoc)}`
      }
      console.log(assemblyOut);
    }
  } else {
    for (let j = 0; j < processString.length; j++) {
      console.log(`${opCommand} 0x${pad((startingAddress + j).toString(16), padding > -1 ? padding : 4)} ${convertToHex(processString[j], j + 1)}`);
    }
    if (stringTerminatorChar) {
      console.log(`${opCommand} 0x${pad((processString.length).toString(16), padding > -1 ? padding : 4)} ${convertToHex(0x00, processString.length)}`);
    }
    console.log("");
  }
}

if (commandCounter > 0 && assemblyOut.length > 3) {
  if (stringTerminatorChar) {
    assemblyOut += `${opCommand} 0x${pad((terminatorLoc).toString(16), padding > -1 ? padding : 4)} ${convertToHex(0x00, terminatorLoc)}`
  }
  console.log(assemblyOut);
}

