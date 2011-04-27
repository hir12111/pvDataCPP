/*PVStructureArray.cpp*/
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstdio>
#include "pvData.h"
#include "convert.h"
#include "factory.h"
#include "serializeHelper.h"
#include "DefaultPVStructureArray.h"

using std::tr1::static_pointer_cast;
using std::tr1::const_pointer_cast;

namespace epics { namespace pvData {

    BasePVStructureArray::BasePVStructureArray(
        PVStructure *parent,StructureArrayConstPtr structureArray)
    : PVStructureArray(parent,structureArray),
      structureArray(structureArray),
      structureArrayData(new StructureArrayData()),
      value(new PVStructurePtr[0])
     {
     }

    BasePVStructureArray::~BasePVStructureArray()
    {
        delete structureArrayData;
        int number = getCapacity();
        for(int i=0; i<number; i++) {
           if(value[i]!=0) {
                delete value[i];
           }
        }
        delete[] value;
    }

    int BasePVStructureArray::append(int number)
    {
        int currentLength = getCapacity();
        int newLength = currentLength + number;
        setCapacity(newLength);
        StructureConstPtr structure = structureArray->getStructure();
        for(int i=currentLength; i<newLength; i++) {
            value[i] =  getPVDataCreate()->createPVStructure(0,structure);
        }
        return newLength;
    }

    bool BasePVStructureArray::remove(int offset,int number)
    {
        int length = getCapacity();
        if(offset+number>length) return false;
        for(int i=offset;i<offset+number;i++) {
           if(value[i]!=0) {
                delete value[i];
                value[i] = 0;
           }
        }
        return true;
    }

    void BasePVStructureArray::compress() {
        int length = getCapacity();
        int newLength = 0;
        for(int i=0; i<length; i++) {
            if(value[i]!=0) {
                newLength++;
                continue;
            }
            // find first non 0
            int notNull = 0;
            for(int j=i+1;j<length;j++) {
                if(value[j]!=0) {
                    notNull = j;
                    break;
                }
            }
            if(notNull!=0) {
                value[i] = value[notNull];
                value[notNull] = 0;
                newLength++;
                continue;
             }
             break;
        }
        setCapacity(newLength);
    }

    void BasePVStructureArray::setCapacity(int capacity) {
        if(getCapacity()==capacity) return;
        if(!isCapacityMutable()) {
            std::string message("not capacityMutable");
            PVField::message(message, errorMessage);
            return;
        }
        int length = getCapacity();
        int numRemove = length - capacity;
        if(numRemove>0) remove(length,numRemove);
        PVStructurePtrArray  newValue = new PVStructurePtr[capacity];
        int limit = length;
        if(length>capacity) limit = capacity;
        for(int i=0; i<limit; i++) newValue[i] = value[i];
        for(int i=limit; i<capacity; i++) newValue[i] = 0;
        if(length>capacity) length = capacity;
        delete[] value;
        value = newValue;
        setCapacityLength(capacity,length);
    }


    StructureArrayConstPtr BasePVStructureArray::getStructureArray()
    {
        return structureArray;
    }

    int BasePVStructureArray::get(
        int offset, int len, StructureArrayData *data)
    {
        int n = len;
        int length = getLength();
        if(offset+len > length) {
            n = length - offset;
            if(n<0) n = 0;
        }
        data->data = value;
        data->offset = offset;
        return n;
    }

    int BasePVStructureArray::put(int offset,int len,
        PVStructurePtrArray  from, int fromOffset)
    {
        if(isImmutable()) {
            message(String("field is immutable"), errorMessage);
            return 0;
        }
        if(from==value) return len;
        if(len<1) return 0;
        int length = getLength();
        int capacity = getCapacity();
        if(offset+len > length) {
            int newlength = offset + len;
            if(newlength>capacity) {
                setCapacity(newlength);
                capacity = getCapacity();
                newlength = capacity;
                len = newlength - offset;
                if(len<=0) return 0;
            }
            length = newlength;
            setLength(length);
        }
        StructureConstPtr structure = structureArray->getStructure();
        for(int i=0; i<len; i++) {
                if(value[i+offset]!=0) delete value[i+offset];
        	PVStructurePtr frompv = from[i+fromOffset];
        	if(frompv==0) {
        		value[i+offset] = 0;
        		continue;
        	}
        	if(frompv->getStructure()!=structure) {
                     throw std::invalid_argument(String(
                       "Element is not a compatible structure"));
        	}
        	value[i+offset] = frompv;
        }
        postPut();
        return len;
    }

    void BasePVStructureArray::shareData(
        PVStructurePtrArray newValue,int capacity,int length)
    {
        for(int i=0; i<getLength(); i++) {
           if(value[i]!=0) delete value[i];
        }
        delete[] value;
        value = newValue;
        setCapacity(capacity);
        setLength(length);
    }

    void BasePVStructureArray::serialize(ByteBuffer *pbuffer,
            SerializableControl *pflusher) const {
        serialize(pbuffer, pflusher, 0, getLength());
    }

    void BasePVStructureArray::deserialize(ByteBuffer *pbuffer,
            DeserializableControl *pcontrol) {
        int size = SerializeHelper::readSize(pbuffer, pcontrol);
        if(size>=0) {
            // prepare array, if necessary
            if(size>getCapacity()) setCapacity(size);
            for(int i = 0; i<size; i++) {
                pcontrol->ensureData(1);
                int8 temp = pbuffer->getByte();
                if(temp==0) {
                    if (value[i]) {
                        delete value[i];
                        value[i] = NULL;
                    }
                }
                else {
                    if(value[i]==NULL) {
                        value[i] = getPVDataCreate()->createPVStructure(
                                NULL, structureArray->getStructure());
                    }
                    value[i]->deserialize(pbuffer, pcontrol);
                }
            }
            setLength(size);
            postPut();
        }
    }

    void BasePVStructureArray::serialize(ByteBuffer *pbuffer,
            SerializableControl *pflusher, int offset, int count) const {
        // cache
        int length = getLength();

        // check bounds
        if(offset<0)
            offset = 0;
        else if(offset>length) offset = length;
        if(count<0) count = length;

        int maxCount = length-offset;
        if(count>maxCount) count = maxCount;

        // write
        SerializeHelper::writeSize(count, pbuffer, pflusher);
        for(int i = 0; i<count; i++) {
            if(pbuffer->getRemaining()<1) pflusher->flushSerializeBuffer();
            PVStructure* pvStructure = value[i+offset];
            if(pvStructure==NULL) {
                pbuffer->putByte(0);
            }
            else {
                pbuffer->putByte(1);
                pvStructure->serialize(pbuffer, pflusher);
            }
        }
    }

}}
