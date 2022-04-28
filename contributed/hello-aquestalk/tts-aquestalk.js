import AudioOut from "pins/audioout";
import Timer from 'timer'

class Aques @ "xs_aques_destructor" {
	constructor(dictionary) @ "xs_aques";
  setSpeech(speech) @ "xs_aques_set_speech";
  syntheFrame() @"xs_aques_synthe_frame";
}
Object.freeze(Aques.prototype);


const LEN = 8
const TIMEOUT = 50000

let samples = null;
let promise
let handler
let syntheRoop

let aques = null;

function enqueueWait(audioOut, ...values) {
  /* keep retrying until queue is not full */
  // eslint-disable-next-line no-constant-condition
  while (true) {
    try {
      audioOut.enqueue(...values)
      break
    } catch (e) {
      if (e.message !== '(host) : queue full') {
        throw e
      }
    }
  }
}


class TTS {
  
  static async speak(aquesText, speed, volume) {
    if (handler != null) {
      return -1
    }
    if (aques == null) {
      aques = new Aques();
      trace("Aques Initialized!\n")
    }

    aques.setSpeech(aquesText, speed)
    trace(` aques.setSpeech : ${aquesText} speed : ${speed}\n`)

    let audioOut = new AudioOut({
      streams: 1,
      bitsPerSample: 16,
      numChannels: 1,
      sampleRate: 8000,
    })

    if (samples == null) {
      samples = []
      try {
        while (true) {
          const s = new SharedArrayBuffer(4096);
          samples.push(s);
          if (samples.length > LEN)
            break;
        }
        trace(" buffer ready!\n")
      } catch(e) {
        trace(e.message)
      }
    }

    promise = new Promise((resolve, reject) => {
      
      let bytes = 0
      
      const cleanup = () => {
        trace(' cleaning up!\n')
        if (handler != null) {
          Timer.clear(handler)
          handler = null
        }
        if (syntheRoop != null) {
          Timer.clear(syntheRoop)
          syntheRoop = null
        }
        audioOut.close()
        audioOut = null
        samples = null
      }
      const onFinish = () => {
        trace(' onFinish :)\n')
        cleanup()
        resolve(bytes)
      }
      const onReject = () => {
        trace(' onReject :(n')
        cleanup()
        reject(-1)
      }
      handler = Timer.set(() => {
        handler = null
        onReject()
      }, TIMEOUT)

      audioOut.start()
      audioOut.enqueue(0, AudioOut.Flush)
      if (volume == undefined) {
        volume = 64;  
      }
      audioOut.enqueue(0, AudioOut.Volume, volume)
      trace(` audioOut.Volume : ${volume}\n`);
      audioOut.callback = () => {

        let elementCount = audioOut.getElementCount() 
        while (elementCount != 0) {
          trace(` elementCount : ${elementCount}\n`);
          elementCount = audioOut.getElementCount();
        }

        trace(` all samples played. ${bytes} bytes!\n`)
        Timer.set(onFinish, 0)
      }

      // playback
      let sfResult = 0;
      let sampleToUse = 0;
      let finished = false;
      let s = samples.shift();

      syntheRoop = Timer.repeat(() => {

        sfResult = aques.syntheFrame(s, sampleToUse);

        if (sfResult < 0 && !finished) {
          trace(" syntheFrame finished!\n")
          finished = true

          // Callback
          enqueueWait(audioOut, 0, AudioOut.Callback, 0)
          //audioOut.enqueue(0, AudioOut.Callback, 0);
        }

        if (!finished) {
          sampleToUse += sfResult
          //trace(` sampleToUse: ${sampleToUse}\n`);
          bytes += sfResult

          let elementCount = audioOut.getElementCount()
          // It should be able to go up to 8, but use 6 for delay measures.
          if (elementCount < 6) {
            // Play the buffered portion
            //audioOut.enqueue(0, AudioOut.RawSamples, s, 1, 0, sampleToUse);
            enqueueWait(audioOut, 0, AudioOut.RawSamples, s, 1, 0, sampleToUse)

            sampleToUse = 0;

            // If it can be playback, replaced with the next area.
            samples.push(s);
            s = samples.shift();
          }else{
            //trace(` wait elementCount:${elementCount} / sampleToUse:${sampleToUse}\n`);
          }
        }
      }, 15)
   })
   return promise
  }
}

export default TTS;