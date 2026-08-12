import inet.common.INETDefs;
import inet.common.Units;
import inet.common.packet.chunk.Chunk;


cplusplus {{
using namespace inet;
}}

namespace flora;

class ChannelBusyHeader extends inet::FieldsChunk {
    int gate;
}