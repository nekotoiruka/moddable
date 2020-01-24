/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include "xsAll.h"

static txInteger fxArgToByteOffset(txMachine* the, txInteger argi, txInteger offset);
static txSlot* fxArgToInstance(txMachine* the, txInteger i);
static txBoolean fxCheckLength(txMachine* the, txSlot* slot, txInteger* index);

static txSlot* fxCheckArrayBufferDetached(txMachine* the, txSlot* slot, txBoolean mutable);
static txSlot* fxCheckArrayBufferInstance(txMachine* the, txSlot* slot);
static txSlot* fxNewArrayBufferInstance(txMachine* the);

static txSlot* fxCheckDataViewInstance(txMachine* the, txSlot* slot);
static txSlot* fxNewDataViewInstance(txMachine* the);

static void fxCallTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index, txSlot* item);
static txSlot* fxCheckTypedArrayInstance(txMachine* the, txSlot* slot);
static int fxCompareTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index);
static txSlot* fxConstructTypedArray(txMachine* the);
static txSlot* fxNewTypedArrayInstance(txMachine* the, txTypeDispatch* dispatch, txTypeAtomics* atomics);
static void fxReduceTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index);

static txBoolean fxTypedArrayDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxTypedArrayDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txBoolean fxTypedArrayGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot);
static txSlot* fxTypedArrayGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxTypedArrayGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);
static txBoolean fxTypedArrayHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static void fxTypedArrayOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys);
static txSlot* fxTypedArraySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxTypedArraySetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);

static void fx_TypedArray_from_object(txMachine* the, txSlot* instance, txSlot* function, txSlot* _this);

const txBehavior ICACHE_FLASH_ATTR gxTypedArrayBehavior = {
	fxTypedArrayGetProperty,
	fxTypedArraySetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxTypedArrayDefineOwnProperty,
	fxTypedArrayDeleteProperty,
	fxTypedArrayGetOwnProperty,
	fxTypedArrayGetPropertyValue,
	fxOrdinaryGetPrototype,
	fxTypedArrayHasProperty,
	fxOrdinaryIsExtensible,
	fxTypedArrayOwnKeys,
	fxOrdinaryPreventExtensions,
	fxTypedArraySetPropertyValue,
	fxOrdinarySetPrototype,
};

void fxArrayBuffer(txMachine* the, txSlot* slot, void* data, txInteger byteLength)
{
	txSlot* instance;
	txSlot* arrayBuffer;
	if (byteLength < 0)
		mxRangeError("invalid byteLength %ld", byteLength);
	mxPush(mxArrayBufferPrototype);
	instance = fxNewArrayBufferInstance(the);
	arrayBuffer = instance->next;
	if (byteLength) {
		arrayBuffer->value.arrayBuffer.address = fxNewChunk(the, byteLength);
		arrayBuffer->value.arrayBuffer.length = byteLength;
		if (data != NULL)
			c_memcpy(arrayBuffer->value.arrayBuffer.address, data, byteLength);
		else
			c_memset(arrayBuffer->value.arrayBuffer.address, 0, byteLength);
	}
	mxPullSlot(slot);
}

void fxGetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	if ((byteOffset < 0) || (length < byteOffset))
		mxRangeError("out of range byteOffset %ld", byteOffset);
	if ((byteLength < 0) || (length < (byteOffset + byteLength)))
		mxRangeError("out of range byteLength %ld", byteLength);
	c_memcpy(data, arrayBuffer->value.arrayBuffer.address + byteOffset, byteLength);
}

txInteger fxGetArrayBufferLength(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	return arrayBuffer->value.arrayBuffer.length;
}

void fxSetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	if ((byteOffset < 0) || (length < byteOffset))
		mxRangeError("out of range byteOffset %ld", byteOffset);
	if ((byteLength < 0) || (length < (byteOffset + byteLength)))
		mxRangeError("out of range byteLength %ld", byteLength);
	c_memcpy(arrayBuffer->value.arrayBuffer.address + byteOffset, data, byteLength);
}

void fxSetArrayBufferLength(txMachine* the, txSlot* slot, txInteger target)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	txByte* address = arrayBuffer->value.arrayBuffer.address;
	if (length != target) {
		if (address)
			address = (txByte*)fxRenewChunk(the, address, target);
		if (address) {
			if (length < target)
				c_memset(address + length, 0, target - length);
		}
		else {
			address = (txByte*)fxNewChunk(the, target);
			if (length < target) {
				c_memcpy(address, arrayBuffer->value.arrayBuffer.address, length);
				c_memset(address + length, 0, target - length);
			}
			else
				c_memcpy(address, arrayBuffer->value.arrayBuffer.address, target);
		}
		arrayBuffer->value.arrayBuffer.length = target;
		arrayBuffer->value.arrayBuffer.address = address;
	}
}

void* fxToArrayBuffer(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	return arrayBuffer->value.arrayBuffer.address;
}

void fxBuildDataView(txMachine* the)
{
    txSlot* slot;
	txInteger index;
	const txTypeDispatch *dispatch;
	const txTypeAtomics *atomics;
	txSlot* property;
    txSlot* constructor;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_get_byteLength), C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_concat), 1, mxID(_concat), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_slice), 2, mxID(_slice), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "ArrayBuffer", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxArrayBufferPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_ArrayBuffer), 1, mxID(_ArrayBuffer));
	mxArrayBufferConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_fromBigInt), 1, mxID(_fromBigInt), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_fromString), 1, mxID(_fromString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_isView), 1, mxID(_isView), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getBigInt64), 1, mxID(_getBigInt64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setBigInt64), 2, mxID(_setBigInt64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getBigUint64), 1, mxID(_getBigUint64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setBigUint64), 2, mxID(_setBigUint64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getFloat32), 1, mxID(_getFloat32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setFloat32), 2, mxID(_setFloat32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getFloat64), 1, mxID(_getFloat64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setFloat64), 2, mxID(_setFloat64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getInt8), 1, mxID(_getInt8), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setInt8), 2, mxID(_setInt8), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getInt16), 1, mxID(_getInt16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setInt16), 2, mxID(_setInt16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getInt32), 1, mxID(_getInt32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setInt32), 2, mxID(_setInt32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getUint8), 1, mxID(_getUint8), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setUint8), 2, mxID(_setUint8), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getUint16), 1, mxID(_getUint16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setUint16), 2, mxID(_setUint16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getUint32), 1, mxID(_getUint32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setUint32), 2, mxID(_setUint32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getUint8), 1, mxID(_getUint8Clamped), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setUint8Clamped), 2, mxID(_setUint8Clamped), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_DataView_prototype_buffer_get), C_NULL, mxID(_buffer), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_DataView_prototype_byteLength_get), C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_DataView_prototype_byteOffset_get), C_NULL, mxID(_byteOffset), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "DataView", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxDataViewPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_DataView), 1, mxID(_DataView));
	mxDataViewConstructor = *the->stack;
	the->stack++;
	
	fxNewHostFunction(the, mxCallback(fxTypedArrayGetter), 0, XS_NO_ID);
	fxNewHostFunction(the, mxCallback(fxTypedArraySetter), 1, XS_NO_ID);
	mxPushUndefined();
	the->stack->flag = XS_DONT_DELETE_FLAG;
	the->stack->kind = XS_ACCESSOR_KIND;
	the->stack->value.accessor.getter = (the->stack + 2)->value.reference;
	the->stack->value.accessor.setter = (the->stack + 1)->value.reference;
	mxPull(mxTypedArrayAccessor);
	mxPop();
	mxPop();
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_buffer_get), C_NULL, mxID(_buffer), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_byteLength_get), C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_byteOffset_get), C_NULL, mxID(_byteOffset), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_length_get), C_NULL, mxID(_length), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_toStringTag_get), C_NULL, mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_copyWithin), 2, mxID(_copyWithin), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_entries), 0, mxID(_entries), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_every), 1, mxID(_every), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_fill), 1, mxID(_fill), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_filter), 1, mxID(_filter), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_find), 1, mxID(_find), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_findIndex), 1, mxID(_findIndex), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_forEach), 1, mxID(_forEach), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_includes), 1, mxID(_includes), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_indexOf), 1, mxID(_indexOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_join), 1, mxID(_join), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_keys), 0, mxID(_keys), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_lastIndexOf), 1, mxID(_lastIndexOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_map), 1, mxID(_map), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_reduce), 1, mxID(_reduce), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_reduceRight), 1, mxID(_reduceRight), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_reverse), 0, mxID(_reverse), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_set), 1, mxID(_set), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_slice), 2, mxID(_slice), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_some), 1, mxID(_some), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_sort), 1, mxID(_sort), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_subarray), 2, mxID(_subarray), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_toLocaleString), 0, mxID(_toLocaleString), XS_DONT_ENUM_FLAG);
	property = mxBehaviorGetProperty(the, mxArrayPrototype.value.reference, mxID(_toString), XS_NO_ID, XS_OWN);
	slot = fxNextSlotProperty(the, slot, property, mxID(_toString), XS_DONT_ENUM_FLAG);
	property = slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_values), 0, mxID(_values), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	mxTypedArrayPrototype = *the->stack;	
	constructor = fxBuildHostConstructor(the, mxCallback(fx_TypedArray), 0, mxID(_TypedArray));
	mxTypedArrayConstructor = *the->stack;
	slot = fxLastProperty(the, constructor);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_from), 1, mxID(_from), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_of), 0, mxID(_of), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	for (index = 0, dispatch = &gxTypeDispatches[0], atomics = &gxTypeAtomics[0]; index < mxTypeArrayCount; index++, dispatch++, atomics++) {
		mxPush(mxTypedArrayPrototype);
		slot = fxLastProperty(the, fxNewObjectInstance(the));
		slot = fxNextIntegerProperty(the, slot, dispatch->size, mxID(_BYTES_PER_ELEMENT), XS_GET_ONLY);
		slot = fxBuildHostConstructor(the, mxCallback(fx_TypedArray), 3, mxID(dispatch->constructorID));
		the->stackPrototypes[-1 - dispatch->constructorID] = *the->stack; //@@
		slot->value.instance.prototype = constructor;
		slot = fxLastProperty(the, slot);
		slot = fxNextTypeDispatchProperty(the, slot, (txTypeDispatch*)dispatch, (txTypeAtomics*)atomics, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
		slot = fxNextIntegerProperty(the, slot, dispatch->size, mxID(_BYTES_PER_ELEMENT), XS_GET_ONLY);
		the->stack++;
	}
	the->stack++;
}

txInteger fxArgToByteOffset(txMachine* the, txInteger argi, txInteger offset)
{
	txSlot *arg = mxArgv(argi);
	if ((mxArgc > argi) && (arg->kind != XS_UNDEFINED_KIND)) {
		txNumber value;
		if (XS_INTEGER_KIND == arg->kind) {
			if (arg->value.integer < 0)
				mxRangeError("out of range byteOffset");
			return arg->value.integer;
		}

		value = c_trunc(fxToNumber(the, arg));
		if (c_isnan(value))
			return 0;
		if ((value < 0) || (0x7FFFFFFF < value)) 	//@@ is this correct?? it looks like it overflows a 32 bit signed int
			mxRangeError("out of range byteOffset");
		return (txInteger)value;
	}
	return offset;
}

txInteger fxArgToByteLength(txMachine* the, txInteger argi, txInteger length)
{
	txSlot *arg = mxArgv(argi);
	if ((mxArgc > argi) && (arg->kind != XS_UNDEFINED_KIND)) {
		txNumber value;
		if (XS_INTEGER_KIND == arg->kind) {
			txInteger value = arg->value.integer;
			if (value < 0)
				mxRangeError("out of range byteLength");
			return value;
		}
		value = c_trunc(fxToNumber(the, arg));
		if (c_isnan(value))
			return 0;
		if ((value < 0) || (0x7FFFFFFF < value))
			mxRangeError("out of range byteLength");
		return (txInteger)value;
	}
	return length;
}

txSlot* fxArgToInstance(txMachine* the, txInteger i)
{
	if (mxArgc > i)
		return fxToInstance(the, mxArgv(i));
	mxTypeError("Cannot coerce undefined to object");
	return C_NULL;
}

txBoolean fxCheckLength(txMachine* the, txSlot* slot, txInteger* index)
{
	txNumber number = fxToNumber(the, slot);
	txNumber check = c_trunc(number);
	if ((number == check) && (0 <= number) && (number <= 0x7FFFFFFF)) {
		*index = (txInteger)number;
		return 1 ;
	}
	return 0;
}

txSlot* fxCheckArrayBufferDetached(txMachine* the, txSlot* slot, txBoolean mutable)
{
	slot = slot->value.reference->next;
	if (slot->value.arrayBuffer.address == C_NULL)
		mxTypeError("detached buffer");
	if (mutable && (slot->flag & XS_MARK_FLAG))
		mxTypeError("ArrayBuffer instance is read-only");
	return slot;
}

txSlot* fxCheckArrayBufferInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_ARRAY_BUFFER_KIND))
			return instance;
	}
	mxTypeError("this is no ArrayBuffer instance");
	return C_NULL;
}

void fxConstructArrayBufferResult(txMachine* the, txSlot* constructor, txInteger length)
{
	txSlot* instance;
	if (constructor)
		mxPushSlot(constructor);
	else {
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_constructor));
	}
	fxToSpeciesConstructor(the, &mxArrayBufferConstructor);
	mxNew();
	mxPushInteger(length);
	mxRunCount(1);
	if (the->stack->kind != XS_REFERENCE_KIND)
		mxTypeError("no instance");
	instance = the->stack->value.reference;
	if (!(instance->next) || (instance->next->kind != XS_ARRAY_BUFFER_KIND))
		mxTypeError("no ArrayBuffer instance");
	if (!constructor && (mxThis->value.reference == instance))
		mxTypeError("same ArrayBuffer instance");
	if (instance->next->value.arrayBuffer.length < length)
		mxTypeError("smaller ArrayBuffer instance");
	mxPullSlot(mxResult);
}

txSlot* fxNewArrayBufferInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_ARRAY_BUFFER_KIND;
	property->value.arrayBuffer.address = C_NULL;
	property->value.arrayBuffer.length = 0;
	return instance;
}

void fx_ArrayBuffer(txMachine* the)
{
	txSlot* instance;
	txSlot* arrayBuffer;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: ArrayBuffer");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxArrayBufferPrototype);
	instance = fxNewArrayBufferInstance(the);
	mxPullSlot(mxResult);
	arrayBuffer = instance->next;
	arrayBuffer->value.arrayBuffer.length = fxArgToByteLength(the, 0, 0);
	arrayBuffer->value.arrayBuffer.address = fxNewChunk(the, arrayBuffer->value.arrayBuffer.length);
	c_memset(arrayBuffer->value.arrayBuffer.address, 0, arrayBuffer->value.arrayBuffer.length);
}

void fx_ArrayBuffer_fromBigInt(txMachine* the)
{
	txU4 minBytes = 0;
	txBoolean sign = 0;
	int endian = EndianBig;
	if (mxArgc < 1)
		mxTypeError("no argument");
	if (mxArgc > 1)
		minBytes = fxToInteger(the, mxArgv(1));
	if ((mxArgc > 2) && fxToBoolean(the, mxArgv(2)))
		sign = 1;
	if ((mxArgc > 3) && fxToBoolean(the, mxArgv(3)))
		endian = EndianLittle;
	if (gxTypeBigInt.toArrayBuffer) {
		gxTypeBigInt.toArrayBuffer(the, mxArgv(0), minBytes, sign, endian);
	}
	else {
		mxUnknownError("not built-in");
	}
}

void fx_ArrayBuffer_fromString(txMachine* the)
{
	txInteger length;
	if (mxArgc < 1)
		mxTypeError("no argument");
	length = c_strlen(fxToString(the, mxArgv(0)));
	fxConstructArrayBufferResult(the, mxThis, length);
	c_memcpy(mxResult->value.reference->next->value.arrayBuffer.address, mxArgv(0)->value.string, length);
}


void fx_ArrayBuffer_isView(txMachine* the)
{
	txSlot* slot;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc > 0) {
		slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if (slot->next) {
				slot = slot->next;
				if ((slot->kind == XS_DATA_VIEW_KIND) || (slot->kind == XS_TYPED_ARRAY_KIND)) {
					mxResult->value.boolean = 1;
				}
			}
		}
	}
}

void fx_ArrayBuffer_prototype_get_byteLength(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	fxCheckArrayBufferDetached(the, mxThis, XS_IMMUTABLE);
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = instance->next->value.arrayBuffer.length;
}

void fx_ArrayBuffer_prototype_concat(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	txInteger c = mxArgc, i = 0;
	txByte* address;
	txSlot* slot;
	while (i < c) {
		arrayBuffer = C_NULL;
		slot = mxArgv(i);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference->next;
			if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND))
				arrayBuffer = slot;
		}
		if (arrayBuffer)
			length += arrayBuffer->value.arrayBuffer.length;
		else
			mxTypeError("arguments[%ld] is no ArrayBuffer instance", i);
		i++;
	}
	fxConstructArrayBufferResult(the, C_NULL, length);
	address = mxResult->value.reference->next->value.arrayBuffer.address;
	arrayBuffer = instance->next;
	length = arrayBuffer->value.arrayBuffer.length;
	c_memcpy(address, arrayBuffer->value.arrayBuffer.address, length);
	address += length;
	i = 0;
	while (i < c) {
		arrayBuffer = mxArgv(i)->value.reference->next;
		length = arrayBuffer->value.arrayBuffer.length;
		c_memcpy(address, arrayBuffer->value.arrayBuffer.address, length);
		address += length;
		i++;
	}
}

void fx_ArrayBuffer_prototype_slice(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	txInteger start = (txInteger)fxArgToIndex(the, 0, 0, length);
	txInteger stop = (txInteger)fxArgToIndex(the, 1, length, length);
	if (stop < start) 
		stop = start;
	fxConstructArrayBufferResult(the, C_NULL, stop - start);
	c_memcpy(mxResult->value.reference->next->value.arrayBuffer.address, arrayBuffer->value.arrayBuffer.address + start, stop - start);
}


txSlot* fxCheckDataViewInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_DATA_VIEW_KIND))
			return instance;
	}
	mxTypeError("this is no DataView instance");
	return C_NULL;
}

txSlot* fxNewDataViewInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_GET_ONLY;
	property->kind = XS_DATA_VIEW_KIND;
	property->value.dataView.offset = 0;
	property->value.dataView.size = 0;
	property = fxNextNullProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	return instance;
}

void fx_DataView(txMachine* the)
{
	txSlot* instance;
	txSlot* view;
	txSlot* buffer;
	txSlot* slot;
	txBoolean flag = 0;
	txInteger limit, offset, size;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: DataView");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxDataViewPrototype);
	instance = fxNewDataViewInstance(the);
	mxPullSlot(mxResult);
	view = instance->next;
	buffer = view->next;
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_REFERENCE_KIND)) {
		slot = mxArgv(0)->value.reference->next;
		if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND)) {
			flag = 1;
			limit = slot->value.arrayBuffer.length;
		}
		else if (slot && (slot->kind == XS_HOST_KIND)) {
			mxPushSlot(mxArgv(0));
			fxGetID(the, mxID(_byteLength));
			flag = fxCheckLength(the, the->stack, &limit);
			mxPop();
		}
	}
	if (!flag)
		mxTypeError("buffer is no ArrayBuffer instance");
	offset = fxArgToByteOffset(the, 1, 0);
	fxCheckArrayBufferDetached(the, mxArgv(0), XS_IMMUTABLE);
	if (limit < offset)
		mxRangeError("out of range byteOffset %ld", offset);
	size = fxArgToByteLength(the, 2, limit - offset);
	if (limit < (offset + size))
		mxRangeError("out of range byteLength %ld", size);
	view->value.dataView.offset = offset;
	view->value.dataView.size = size;
	buffer->kind = XS_REFERENCE_KIND;
	buffer->value.reference = mxArgv(0)->value.reference;
}

void fx_DataView_prototype_buffer_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	mxResult->kind = buffer->kind;
	mxResult->value = buffer->value;
}

void fx_DataView_prototype_byteLength_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = view->value.dataView.size;
}

void fx_DataView_prototype_byteOffset_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = view->value.dataView.offset;
}

void fx_DataView_prototype_get(txMachine* the, txNumber delta, txTypeCallback getter)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	txInteger offset = fxArgToByteOffset(the, 0, 0);
	int endian = EndianBig;
	if ((mxArgc > 1) && fxToBoolean(the, mxArgv(1)))
		endian = EndianLittle;
	fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
	if (((view->value.dataView.size < delta) || (view->value.dataView.size - delta) < offset))
		mxRangeError("out of range byteOffset");
	offset += view->value.dataView.offset;
	(*getter)(the, buffer->value.reference->next, offset, mxResult, endian);
}

void fx_DataView_prototype_getBigInt64(txMachine* the)
{
	fx_DataView_prototype_get(the, 8, fxBigInt64Getter);
}

void fx_DataView_prototype_getBigUint64(txMachine* the)
{
	fx_DataView_prototype_get(the, 8, fxBigUint64Getter);
}

void fx_DataView_prototype_getFloat32(txMachine* the)
{
	fx_DataView_prototype_get(the, 4, fxFloat32Getter);
}

void fx_DataView_prototype_getFloat64(txMachine* the)
{
	fx_DataView_prototype_get(the, 8, fxFloat64Getter);
}

void fx_DataView_prototype_getInt8(txMachine* the)
{
	fx_DataView_prototype_get(the, 1, fxInt8Getter);
}

void fx_DataView_prototype_getInt16(txMachine* the)
{
	fx_DataView_prototype_get(the, 2, fxInt16Getter);
}

void fx_DataView_prototype_getInt32(txMachine* the)
{
	fx_DataView_prototype_get(the, 4, fxInt32Getter);
}

void fx_DataView_prototype_getUint8(txMachine* the)
{
	fx_DataView_prototype_get(the, 1, fxUint8Getter);
}

void fx_DataView_prototype_getUint16(txMachine* the)
{
	fx_DataView_prototype_get(the, 2, fxUint16Getter);
}

void fx_DataView_prototype_getUint32(txMachine* the)
{
	fx_DataView_prototype_get(the, 4, fxUint32Getter);
}

void fx_DataView_prototype_set(txMachine* the, txNumber delta, txTypeCoerce coercer, txTypeCallback setter)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	txInteger offset = fxArgToByteOffset(the, 0, 0);
	int endian = EndianBig;
	txSlot* value;
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	value = the->stack;	
	(*coercer)(the, value);
	if ((mxArgc > 2) && fxToBoolean(the, mxArgv(2)))
		endian = EndianLittle;
	fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
	if (((view->value.dataView.size < delta) || (view->value.dataView.size - delta) < offset))
		mxRangeError("out of range byteOffset");
	offset += view->value.dataView.offset;
	(*setter)(the, buffer->value.reference->next, offset, value, endian);
	mxPop();
}

void fx_DataView_prototype_setBigInt64(txMachine* the)
{
	fx_DataView_prototype_set(the, 8, fxBigIntCoerce, fxBigInt64Setter);
}

void fx_DataView_prototype_setBigUint64(txMachine* the)
{
	fx_DataView_prototype_set(the, 8, fxBigIntCoerce, fxBigUint64Setter);
}

void fx_DataView_prototype_setFloat32(txMachine* the)
{
	fx_DataView_prototype_set(the, 4, fxNumberCoerce, fxFloat32Setter);
}

void fx_DataView_prototype_setFloat64(txMachine* the)
{
	fx_DataView_prototype_set(the, 8, fxNumberCoerce, fxFloat64Setter);
}

void fx_DataView_prototype_setInt8(txMachine* the)
{
	fx_DataView_prototype_set(the, 1, fxIntCoerce, fxInt8Setter);
}

void fx_DataView_prototype_setInt16(txMachine* the)
{
	fx_DataView_prototype_set(the, 2, fxIntCoerce, fxInt16Setter);
}

void fx_DataView_prototype_setInt32(txMachine* the)
{
	fx_DataView_prototype_set(the, 4, fxIntCoerce, fxInt32Setter);
}

void fx_DataView_prototype_setUint8(txMachine* the)
{
	fx_DataView_prototype_set(the, 1, fxUintCoerce, fxUint8Setter);
}

void fx_DataView_prototype_setUint16(txMachine* the)
{
	fx_DataView_prototype_set(the, 2, fxUintCoerce, fxUint16Setter);
}

void fx_DataView_prototype_setUint32(txMachine* the)
{
	fx_DataView_prototype_set(the, 4, fxUintCoerce, fxUint32Setter);
}

void fx_DataView_prototype_setUint8Clamped(txMachine* the)
{
	fx_DataView_prototype_set(the, 1, fxNumberCoerce, fxUint8ClampedSetter);
}


#define mxTypedArrayDeclarations \
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis); \
	txSlot* dispatch = instance->next; \
	txSlot* view = dispatch->next; \
	txSlot* buffer = view->next; \
	txSlot* data = fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE); \
	txInteger delta = dispatch->value.typedArray.dispatch->shift; \
	txInteger length = view->value.dataView.size >> delta; \
	delta = dispatch->value.typedArray.dispatch->size

#define mxMutableTypedArrayDeclarations \
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis); \
	txSlot* dispatch = instance->next; \
	txSlot* view = dispatch->next; \
	txSlot* buffer = view->next; \
	txSlot* data = fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE); \
	txInteger delta = dispatch->value.typedArray.dispatch->shift; \
	txInteger length = view->value.dataView.size >> delta; \
	delta = dispatch->value.typedArray.dispatch->size

#define mxResultTypedArrayDeclarations \
	txSlot* resultInstance = fxCheckTypedArrayInstance(the, mxResult); \
	txSlot* resultDispatch = resultInstance->next; \
	txSlot* resultView = resultDispatch->next; \
	txSlot* resultBuffer = resultView->next; \
	txSlot* resultData = fxCheckArrayBufferDetached(the, resultBuffer, XS_MUTABLE); \
	txInteger resultLength = resultView->value.dataView.size >> resultDispatch->value.typedArray.dispatch->shift
	
void fxTypedArrayGetter(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* dispatch;
	while (instance) {
		if (instance->flag & XS_EXOTIC_FLAG) {
			dispatch = instance->next;
			if (dispatch->ID == XS_TYPED_ARRAY_BEHAVIOR)
				break;
		}
		instance = fxGetPrototype(the, instance);
	}
	if (instance) {
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txSlot* data = fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = view->value.dataView.size >> shift;
		txIndex index = the->scratch.value.at.index;
		if (index < length) {
			(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index << shift), mxResult, EndianNative);
		}
	}
}

void fxTypedArraySetter(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* dispatch;
	while (instance) {
		if (instance->flag & XS_EXOTIC_FLAG) {
			dispatch = instance->next;
			if (dispatch->ID == XS_TYPED_ARRAY_BEHAVIOR)
				break;
		}
		instance = fxGetPrototype(the, instance);
	}
	if (instance) {
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txSlot* data = C_NULL;
		txIndex length = view->value.dataView.size >> shift;
		txIndex index = the->scratch.value.at.index;
		txSlot* slot = mxArgv(0);
		dispatch->value.typedArray.dispatch->coerce(the, slot);
		data = fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
		if (index < length) {
			(*dispatch->value.typedArray.dispatch->setter)(the, data, view->value.dataView.offset + (index << shift), slot, EndianNative);
		}
	}
}

txBoolean fxTypedArrayDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask) 
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = view->value.dataView.size >> shift;
		if (index >= length)
			return 0;
		if (mask & XS_ACCESSOR_FLAG)
			return 0;
		if ((mask & XS_DONT_DELETE_FLAG) && (!(slot->flag & XS_DONT_DELETE_FLAG)))
			return 0;
		if ((mask & XS_DONT_ENUM_FLAG) && (slot->flag & XS_DONT_ENUM_FLAG))
			return 0;
		if ((mask & XS_DONT_SET_FLAG) && (slot->flag & XS_DONT_SET_FLAG))
			return 0;
		if (slot->kind != XS_UNINITIALIZED_KIND) {
			txSlot* data;
			dispatch->value.typedArray.dispatch->coerce(the, slot);
			data = fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
			(*dispatch->value.typedArray.dispatch->setter)(the, data, view->value.dataView.offset + (index << shift), slot, EndianNative);
		}
		return 1;
	}
	return fxOrdinaryDefineOwnProperty(the, instance, id, index, slot, mask);
}

txBoolean fxTypedArrayDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = view->value.dataView.size >> shift;
		return (index < length) ? 0 : 1;
	}
	return fxOrdinaryDeleteProperty(the, instance, id, index);
}

txBoolean fxTypedArrayGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = view->value.dataView.size >> shift;
		if (index < length) {
			txSlot* data = fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
			(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index << shift), slot, EndianNative);
			slot->flag = XS_DONT_DELETE_FLAG;
			return 1;
		}
		slot->kind = XS_UNDEFINED_KIND;
		slot->flag = XS_NO_FLAG;
		return 0;
	}
	return fxOrdinaryGetOwnProperty(the, instance, id, index, slot);
}

txSlot* fxTypedArrayGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		the->scratch.value.at.index = index;
		return &mxTypedArrayAccessor;
	}
	return fxOrdinaryGetProperty(the, instance, id, index, flag);
}

txBoolean fxTypedArrayGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txSlot* data = fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = view->value.dataView.size / shift;
		if (index < length) {
			(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index << shift), value, EndianNative);
			return 1;
		}
		value->kind = XS_UNDEFINED_KIND;
		return 0;
	}
	return fxOrdinaryGetPropertyValue(the, instance, id, index, receiver, value);
}

txBoolean fxTypedArrayHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = view->value.dataView.size >> shift;
		fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
		return (index < length) ? 1 : 0;
	}
	return fxOrdinaryHasProperty(the, instance, id, index);
}

void fxTypedArrayOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys)
{
	if (flag & XS_EACH_NAME_FLAG) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = view->value.dataView.size >> shift;
		if (length) {
			txIndex index;
			fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
			for (index = 0; index < length; index++)
				keys = fxQueueKey(the, 0, index, keys);
		}
	}
	fxOrdinaryOwnKeys(the, instance, flag, keys);
}

txSlot* fxTypedArraySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		the->scratch.value.at.index = index;
		return &mxTypedArrayAccessor;
	}
	return fxOrdinarySetProperty(the, instance, id, index, flag);
}

txBoolean fxTypedArraySetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txSlot* data = C_NULL;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = view->value.dataView.size >> shift;
		dispatch->value.typedArray.dispatch->coerce(the, value);
		data = fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
		if (index < length) {
			(*dispatch->value.typedArray.dispatch->setter)(the, data, view->value.dataView.offset + (index << shift), value, EndianNative);
			return 1;
		}
		return 0;
	}
	return fxOrdinarySetPropertyValue(the, instance, id, index, value, receiver);
}

void fxCallTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index, txSlot* item)
{
	if (data->value.arrayBuffer.address == C_NULL)
		mxTypeError("detached buffer");
	/* THIS */
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(function);
	mxCall();
	/* ARGUMENTS */
	mxPushUndefined();
	(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index << dispatch->value.typedArray.dispatch->shift), the->stack, EndianNative);
	if (item) {
		item->kind = the->stack->kind;
		item->value = the->stack->value;
	}
	mxPushInteger(index);
	mxPushSlot(mxThis);
	mxRunCount(3);
}

txSlot* fxCheckTypedArrayInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
        txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_TYPED_ARRAY_KIND))
			return instance;
	}
	mxTypeError("this is no TypedArray instance");
	return C_NULL;
}

int fxCompareTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index)
{
	txSlot* slot = the->stack;
	int result;
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(function);
	mxCall();
	/* ARGUMENTS */
	mxPushUndefined();
	(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index << dispatch->value.typedArray.dispatch->shift), the->stack, EndianNative);
	mxPushSlot(slot);
	mxRunCount(2);
	if (the->stack->kind == XS_INTEGER_KIND)
		result = the->stack->value.integer;
	else {
		txNumber number = fxToNumber(the, the->stack);
		result = (number < 0) ? -1 :  (number > 0) ? 1 : 0;
	}
	the->stack++;
	if (data->value.arrayBuffer.address == C_NULL)
		mxTypeError("detached buffer");
	return result;
}

txSlot* fxConstructTypedArray(txMachine* the)
{
	txSlot* prototype;
	txSlot* dispatch;
	txSlot* instance;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: TypedArray");
	prototype = mxBehaviorGetProperty(the, mxFunction->value.reference, mxID(_prototype), XS_NO_ID, XS_ANY);
	dispatch = prototype->next;
	if (!dispatch || (dispatch->kind != XS_TYPED_ARRAY_KIND))
		mxTypeError("new: TypedArray");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, prototype);
	instance = fxNewTypedArrayInstance(the, dispatch->value.typedArray.dispatch, dispatch->value.typedArray.atomics);
	mxPullSlot(mxResult);
	return instance;
}

void fxCreateTypedArraySpecies(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* constructor = &the->stackPrototypes[-1 - dispatch->value.typedArray.dispatch->constructorID];
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor));
	fxToSpeciesConstructor(the, constructor);
	mxNew();
}

txSlot* fxGetTypedArrayValue(txMachine* the, txSlot* instance, txInteger index)
{
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* data = mxIsReference(buffer) ? fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE) : C_NULL;
	txU2 shift = dispatch->value.typedArray.dispatch->shift;
	index <<= shift;
	if ((0 <= index) && ((index + (1 << shift)) <= view->value.dataView.size)) {
		(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + index, &(the->scratch), EndianNative);
		return &the->scratch;
	}
	return C_NULL;
}

void fxReduceTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index)
{
	if (data->value.arrayBuffer.address == C_NULL)
		mxTypeError("detached buffer");
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(function);
	mxCall();
	/* ARGUMENTS */
	mxPushSlot(mxResult);
	mxPushUndefined();
	(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index << dispatch->value.typedArray.dispatch->shift), the->stack, EndianNative);
	mxPushInteger(index);
	mxPushSlot(mxThis);
	mxRunCount(4);
}

txSlot* fxNewTypedArrayInstance(txMachine* the, txTypeDispatch* dispatch, txTypeAtomics* atomics)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_EXOTIC_FLAG;
	property = fxNextTypeDispatchProperty(the, instance, dispatch, atomics, XS_TYPED_ARRAY_BEHAVIOR, XS_INTERNAL_FLAG | XS_GET_ONLY);
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_GET_ONLY;
	property->kind = XS_DATA_VIEW_KIND;
	property->value.dataView.offset = 0;
	property->value.dataView.size = 0;
	property = fxNextNullProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	return instance;
}

txSlot* fxSetTypedArrayValue(txMachine* the, txSlot* instance, txInteger index)
{
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* data;
	txU2 shift = dispatch->value.typedArray.dispatch->shift;
	index <<= shift;
	(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
	data = fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
	if ((0 <= index) && ((index + (1 << shift)) <= view->value.dataView.size))
		(*dispatch->value.typedArray.dispatch->setter)(the, data, view->value.dataView.offset + index, the->stack, EndianNative);
	the->scratch.kind = XS_UNDEFINED_KIND;
	the->scratch.flag= 0;
	return &the->scratch;
}

void fx_TypedArray(txMachine* the)
{
	txSlot* instance = fxConstructTypedArray(the);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* data = C_NULL;
	txU2 shift = dispatch->value.typedArray.dispatch->shift;
	txSlot* slot;
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_REFERENCE_KIND)) {
		slot = mxArgv(0)->value.reference->next;
		if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND)) {
			txInteger offset = fxArgToByteOffset(the, 1, 0);
			txInteger size;
			txInteger limit;
			if (offset & ((1 << shift) - 1))
				mxRangeError("invalid byteOffset %ld", offset);
			size = fxArgToByteOffset(the, 2, -1);
			fxCheckArrayBufferDetached(the, mxArgv(0), XS_IMMUTABLE);
			limit = slot->value.arrayBuffer.length;
			if (size >= 0) {
				size <<= shift;
				if (limit < (offset + size))
					mxRangeError("out of range byteLength %ld", size);
			}
			else {
				if (limit & ((1 << shift) - 1))
					mxRangeError("invalid byteLength %ld", limit);
				size = limit - offset;
				if (size < 0)
					mxRangeError("out of range byteLength %ld", size);
			}
			view->value.dataView.offset = offset;
			view->value.dataView.size = size;
			buffer->kind = XS_REFERENCE_KIND;
			buffer->value.reference = mxArgv(0)->value.reference;
		}
		else if (slot && (slot->kind == XS_TYPED_ARRAY_KIND)) {
			txSlot* arrayDispatch = slot;
			txSlot* arrayView = arrayDispatch->next;
			txSlot* arrayBuffer = arrayView->next;
			txSlot* arrayData = fxCheckArrayBufferDetached(the, arrayBuffer, XS_IMMUTABLE);
			txInteger arrayDelta = arrayDispatch->value.typedArray.dispatch->size;
			txInteger arrayLength = arrayView->value.dataView.size >> arrayDispatch->value.typedArray.dispatch->shift;
			txInteger arrayOffset = arrayView->value.dataView.offset;
			txInteger offset = 0;
			txInteger size = arrayLength << shift;
			/* THIS */
			mxPushUninitialized();	
			/* FUNCTION */
			mxPush(mxArrayBufferConstructor);
			/* TARGET */
			if (arrayData->kind == XS_ARRAY_BUFFER_KIND) {
				mxPushSlot(arrayBuffer);
				fxGetID(the, mxID(_constructor));
				fxToSpeciesConstructor(the, &mxArrayBufferConstructor);
			}
			else
				mxPush(mxArrayBufferConstructor);
			/* RESULT */
			mxPushUndefined();	
			mxPushUninitialized();	
			mxPushUninitialized();	
			/* ARGUMENTS */
			mxPushInteger(size);
			mxRunCount(1);
			mxPullSlot(buffer);
			
			arrayData = fxCheckArrayBufferDetached(the, arrayBuffer, XS_IMMUTABLE);
			data = fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
			view->value.dataView.offset = offset;
			view->value.dataView.size = size;
			if (dispatch == arrayDispatch)
				c_memcpy(data->value.arrayBuffer.address + offset, arrayData->value.arrayBuffer.address + arrayOffset, size);
			else {
				mxPushUndefined();
				while (offset < size) {
					(*arrayDispatch->value.typedArray.dispatch->getter)(the, arrayData, arrayOffset, the->stack, EndianNative);
					(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
					(*dispatch->value.typedArray.dispatch->setter)(the, data, offset, the->stack, EndianNative);
					arrayOffset += arrayDelta;
					offset += 1 << shift;
				}
				the->stack++;
			}
		}
		else if (slot && (slot->kind == XS_HOST_KIND)) {
			txInteger limit;
			mxPushSlot(mxArgv(0));
			fxGetID(the, mxID(_byteLength));
			if (fxCheckLength(the, the->stack, &limit)) {
				txInteger offset = fxArgToByteOffset(the, 1, 0);
				txInteger size;
				if (offset & ((1 << shift) - 1))
					mxRangeError("invalid byteOffset %ld", offset);
				size = fxArgToByteOffset(the, 2, -1);
				if (size >= 0) {
					size <<= shift;
					if (limit < (offset + size))
						mxRangeError("out of range byteLength %ld", size);
				}
				else {
					if (limit & ((1 << shift) - 1))
						mxRangeError("invalid byteLength %ld", limit);
					size = limit - offset;
					if (size < 0)
						mxRangeError("out of range byteLength %ld", size);
				}
				view->value.dataView.offset = offset;
				view->value.dataView.size = size;
				buffer->kind = XS_REFERENCE_KIND;
				buffer->value.reference = mxArgv(0)->value.reference;
			}
			else
				fx_TypedArray_from_object(the, instance, C_NULL, C_NULL);
			mxPop();
		}
		else {
			fx_TypedArray_from_object(the, instance, C_NULL, C_NULL);
		}
	}
	else {
        txInteger length = fxArgToByteLength(the, 0, 0);
        if (length & (((1 << shift) - 1) << (32 - shift)))
			mxRangeError("out of range byteLength");
        length <<= shift;
		mxPush(mxArrayBufferConstructor);
		mxNew();
		mxPushInteger(length);
		mxRunCount(1);
		mxPullSlot(buffer);
        view->value.dataView.offset = 0;
        view->value.dataView.size = length;
	}
}

void fx_TypedArray_from(txMachine* the)
{
	txSlot* function = C_NULL;
	txSlot* _this = C_NULL;
	if (!mxIsReference(mxThis) || !(mxIsConstructor(mxThis->value.reference)))
		mxTypeError("this is no constructor");
	if (mxArgc > 1) {
		txSlot* slot = mxArgv(1);
		if (!mxIsUndefined(slot)) {
			function = slot;
			if (!fxIsCallable(the, function))
				mxTypeError("map is no function");
			if (mxArgc > 2)
				_this = mxArgv(2);
		}
	}
	fx_TypedArray_from_object(the, C_NULL, function, _this);
}

void fx_TypedArray_from_object(txMachine* the, txSlot* instance, txSlot* function, txSlot* _this)
{
	txSlot* stack = the->stack;
	txSlot* iterator;
	txSlot* next;
	txSlot* value;
	txSlot* list = C_NULL;
	txSlot* slot;
	txSlot* dispatch;
	txSlot* view;
	txSlot* buffer;
	txSlot* data;
	txU2 shift;
	txNumber length;
	mxTemporary(iterator);
	mxTemporary(next);
	if (fxGetIterator(the, mxArgv(0), iterator, next, 1)) {
		list = fxNewInstance(the);
		slot = list;
		length = 0;
		mxTemporary(value);
		while (fxIteratorNext(the, iterator, next, value)) {
			slot = fxNextSlotProperty(the, slot, value, XS_NO_ID, XS_NO_FLAG);
			length++;
		}
		the->stack += 2;
	}
	else {
		mxPushSlot(mxArgv(0));
		fxGetID(the, mxID(_length));
		length = fxToLength(the, the->stack);
		mxPop();
	}
	if (instance) {
		dispatch = instance->next;
		view = dispatch->next;
		buffer = view->next;
		shift = dispatch->value.typedArray.dispatch->shift;
		mxPush(mxArrayBufferConstructor);
		mxNew();
		mxPushNumber(length * dispatch->value.typedArray.dispatch->size);
		mxRunCount(1);
		mxPullSlot(buffer);
		data = fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
		view->value.dataView.offset = 0;
		view->value.dataView.size = data->value.arrayBuffer.length;
	}
	else {
		mxPushSlot(mxThis);
		mxNew();
		mxPushNumber(length);
		mxRunCount(1);
		mxPullSlot(mxResult);
		instance = fxToInstance(the, mxResult);
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_TYPED_ARRAY_KIND)) {
			dispatch = instance->next;
			view = dispatch->next;
			buffer = view->next;
			data = fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
			shift = dispatch->value.typedArray.dispatch->shift;
			if (view->value.dataView.size < (length * dispatch->value.typedArray.dispatch->size))
				mxTypeError("too small TypedArray");
		}
		else
			mxTypeError("no TypedArray");
	}
	if (list) {
		txInteger index = 0;
		slot = list->next;
		while (slot) {
			/* ARG0 */
			if (function) {
				/* THIS */
				if (_this)
					mxPushSlot(_this);
				else
					mxPushUndefined();
				/* FUNCTION */
				mxPushSlot(function);
				mxCall();
				/* ARGUMENTS */
				mxPushSlot(slot);
				mxPushInteger(index);
				mxRunCount(2);
			}
			else
				mxPushSlot(slot);
			(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
			(*dispatch->value.typedArray.dispatch->setter)(the, data, (index << shift), the->stack, EndianNative);
			mxPop();
			index++;
			slot = slot->next;
		}
	}
	else {
		txInteger index = 0;
		txInteger count = (txInteger)length;
		while (index < count) {
			if (function) {
				/* THIS */
				if (_this)
					mxPushSlot(_this);
				else
					mxPushUndefined();
				/* FUNCTION */
				mxPushSlot(function);
				mxCall();
				/* ARGUMENTS */
				mxPushSlot(mxArgv(0));
				fxGetID(the, index);
				mxPushInteger(index);
				mxRunCount(2);
			}
			else {
				mxPushSlot(mxArgv(0));
				fxGetID(the, index);
			}
			(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
			(*dispatch->value.typedArray.dispatch->setter)(the, data, (index << shift), the->stack, EndianNative);
			mxPop();
			index++;
		}	
	}
	the->stack = stack;
}

void fx_TypedArray_of(txMachine* the)
{
	txInteger count = mxArgc;
	txInteger index = 0;
	mxPushSlot(mxThis);
	mxNew();
	mxPushInteger(count);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		mxResultTypedArrayDeclarations;
		txU2 shift = resultDispatch->value.typedArray.dispatch->shift;
		if (resultLength < count)
			mxTypeError("insufficient TypedArray");
		while (index < count) {
			(*resultDispatch->value.typedArray.dispatch->coerce)(the, mxArgv(index));
			(*resultDispatch->value.typedArray.dispatch->setter)(the, resultData, resultView->value.dataView.offset + (index << shift), mxArgv(index), EndianNative);
			index++;
		}
	}
}

void fx_TypedArray_prototype_buffer_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	mxResult->kind = buffer->kind;
	mxResult->value = buffer->value;
}

void fx_TypedArray_prototype_byteLength_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = (buffer->value.reference->next->value.arrayBuffer.address == C_NULL) ? 0 : view->value.dataView.size;
}

void fx_TypedArray_prototype_byteOffset_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = (buffer->value.reference->next->value.arrayBuffer.address == C_NULL) ? 0 : view->value.dataView.offset;
}

void fx_TypedArray_prototype_copyWithin(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txByte* address = data->value.arrayBuffer.address + view->value.dataView.offset;
	txInteger target = (txInteger)fxArgToIndex(the, 0, 0, length);
	txInteger start = (txInteger)fxArgToIndex(the, 1, 0, length);
	txInteger end = (txInteger)fxArgToIndex(the, 2, length, length);
	txInteger count = end - start;
	fxCheckArrayBufferDetached(the, buffer, XS_MUTABLE);
	if (count > length - target)
		count = length - target;
	if (count > 0)
		c_memmove(address + (target * delta), address + (start * delta), count * delta);
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_entries(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* property;
	fxCheckArrayBufferDetached(the, instance->next->next->next, XS_IMMUTABLE);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis));
	property = fxNextIntegerProperty(the, property, 2, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_every(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
		mxResult->value.boolean = fxToBoolean(the, the->stack++);
		if (!mxResult->value.boolean)
			break;
		index++;
	}
}

void fx_TypedArray_prototype_fill(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txInteger start = (txInteger)fxArgToIndex(the, 1, 0, length);
	txInteger end = (txInteger)fxArgToIndex(the, 2, length, length);
	start *= delta;
	end *= delta;
	start += view->value.dataView.offset;
	end += view->value.dataView.offset;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
	while (start < end) {
		(*dispatch->value.typedArray.dispatch->setter)(the, data, start, the->stack, EndianNative);
		start += delta;
	}
	the->stack++;
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_filter(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txInteger count = 0;
	txInteger index = 0;
	mxPushUndefined();
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, the->stack);
		if (fxToBoolean(the, the->stack++)) {
			count++;
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
		}
		index++;
	}
	the->stack++;
	fxCreateTypedArraySpecies(the);
	mxPushNumber(count);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		mxResultTypedArrayDeclarations;
		txInteger resultOffset = 0;
		txInteger resultSize = resultDispatch->value.typedArray.dispatch->size;
		if (resultLength < count)
			mxTypeError("insufficient buffer");
		slot = list->next;
		while (slot) {
			(*resultDispatch->value.typedArray.dispatch->coerce)(the, slot);
			(*resultDispatch->value.typedArray.dispatch->setter)(the, resultData, resultOffset, slot, EndianNative);
			resultOffset += resultSize;
			slot = slot->next;
		}
	}
	the->stack++;
}

void fx_TypedArray_prototype_find(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxPushUndefined();
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, the->stack);
		if (fxToBoolean(the, the->stack++)) {
			mxResult->kind = the->stack->kind;
			mxResult->value = the->stack->value;
			break;
		}
		index++;
	}
	the->stack++;
}

void fx_TypedArray_prototype_findIndex(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = -1;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
		if (fxToBoolean(the, the->stack)) {
			mxResult->value.integer = index;
			break;
		}
		the->stack++;
		index++;
	}
}

void fx_TypedArray_prototype_forEach(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
		the->stack++;
		index++;
	}
}

void fx_TypedArray_prototype_includes(txMachine* the)
{
	mxTypedArrayDeclarations;
	fxBoolean(the, mxResult, 0);
	if (length) {
		txInteger index = (txInteger)fxArgToIndex(the, 1, 0, length);
		if (mxArgc > 0)
			mxPushSlot(mxArgv(0));
		else
			mxPushUndefined();
		mxPushUndefined();
		while (index < length) {
			(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index * delta), the->stack, EndianNative);
			if (fxIsSameValue(the, the->stack, the->stack + 1, 1)) {
				mxResult->value.boolean = 1;
				break;
			}
			index++;
		}
		the->stack += 2;
	}
}

void fx_TypedArray_prototype_indexOf(txMachine* the)
{
	mxTypedArrayDeclarations;
	fxInteger(the, mxResult, -1);
	if (length) {
		txInteger index = (txInteger)fxArgToIndex(the, 1, 0, length);
		if (mxArgc > 0)
			mxPushSlot(mxArgv(0));
		else
			mxPushUndefined();
		mxPushUndefined();
		while (index < length) {
			(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index * delta), the->stack, EndianNative);
			if (fxIsSameSlot(the, the->stack, the->stack + 1)) {
				mxResult->value.integer = index;
				break;
			}
			index++;
		}
		the->stack += 2;
	}
}

void fx_TypedArray_prototype_join(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* data = fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txInteger offset = view->value.dataView.offset;
	txInteger limit = offset + view->value.dataView.size;
	txString string;
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txBoolean comma = 0;
	txInteger size = 0;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		mxPushSlot(mxArgv(0));
		string = fxToString(the, the->stack);
		the->stack->kind += XS_KEY_KIND - XS_STRING_KIND;
		the->stack->value.key.sum = c_strlen(the->stack->value.string);
	}
	else {
		mxPushStringX(",");
		the->stack->kind += XS_KEY_KIND - XS_STRING_KIND;
		the->stack->value.key.sum = 1;
	}
	while (offset < limit) {
		if (comma) {
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			size += slot->value.key.sum;
		}
		else
			comma = 1;
		mxPushUndefined();
		(*dispatch->value.typedArray.dispatch->getter)(the, data, offset, the->stack, EndianNative);
		slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
		string = fxToString(the, slot);
		slot->kind += XS_KEY_KIND - XS_STRING_KIND;
		slot->value.key.sum = c_strlen(string);
		size += slot->value.key.sum;
		mxPop();
		offset += delta;
	}
	mxPop();
	string = mxResult->value.string = fxNewChunk(the, size + 1);
	slot = list->next;
	while (slot) {
		c_memcpy(string, slot->value.key.string, slot->value.key.sum);
		string += slot->value.key.sum;
		slot = slot->next;
	}
	*string = 0;
	mxResult->kind = XS_STRING_KIND;
	mxPop();
}

void fx_TypedArray_prototype_keys(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* property;
	fxCheckArrayBufferDetached(the, instance->next->next->next, XS_IMMUTABLE);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis));
	property = fxNextIntegerProperty(the, property, 1, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_lastIndexOf(txMachine* the)
{
	mxTypedArrayDeclarations;
	fxInteger(the, mxResult, -1);
	if (length) {
		txIndex index = (txIndex)fxArgToLastIndex(the, 1, length, length);
		if (mxArgc > 0)
			mxPushSlot(mxArgv(0));
		else
			mxPushUndefined();
		mxPushUndefined();
		while (index > 0) {
			index--;
			(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index * delta), the->stack, EndianNative);
			if (fxIsSameSlot(the, the->stack, the->stack + 1)) {
				mxResult->value.integer = index;
				break;
			}
		}
		the->stack += 2;
	}
}

void fx_TypedArray_prototype_length_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = (buffer->value.reference->next->value.arrayBuffer.address == C_NULL) ? 0 : view->value.dataView.size >> dispatch->value.typedArray.dispatch->shift;
}

void fx_TypedArray_prototype_map(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	fxCreateTypedArraySpecies(the);
	mxPushNumber(length);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		mxResultTypedArrayDeclarations;
		txU2 shift = resultDispatch->value.typedArray.dispatch->shift;
		txInteger index = 0;
		if (resultLength < length)
			mxTypeError("insufficient buffer");
		while (index < length) {
			fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
			if (resultData->value.arrayBuffer.address == C_NULL)
				mxTypeError("detached buffer");
			(*resultDispatch->value.typedArray.dispatch->coerce)(the, the->stack);
			(*resultDispatch->value.typedArray.dispatch->setter)(the, resultData, resultView->value.dataView.offset + (index << shift), the->stack, EndianNative);
			the->stack++;
			index++;
		}
	}
}

void fx_TypedArray_prototype_reduce(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else if (index < length) {
		(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset, mxResult, EndianNative);
		index++;
	}
	else
		mxTypeError("no initial value");
	while (index < length) {
		fxReduceTypedArrayItem(the, function, dispatch, view, data, index);
		mxPullSlot(mxResult);
		index++;
	}
}

void fx_TypedArray_prototype_reduceRight(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = length - 1;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else if (index >= 0) {
		(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (index * delta), mxResult, EndianNative);
		index--;
	}
	else
		mxTypeError("no initial value");
	while (index >= 0) {
		fxReduceTypedArrayItem(the, function, dispatch, view, data, index);
		mxPullSlot(mxResult);
		index--;
	}
}

void fx_TypedArray_prototype_reverse(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	if (length) {
		txByte buffer;
		txByte* first = data->value.arrayBuffer.address + view->value.dataView.offset;
		txByte* last = first + view->value.dataView.size - delta;
		txInteger offset;
		while (first < last) {
			for (offset = 0; offset < delta; offset++) {
				buffer = last[offset];
				last[offset] = first[offset];
				first[offset] = buffer;
			}
			first += delta;
			last -= delta;
		}
	}
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_set(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txSlot* array = fxArgToInstance(the, 0);
	txInteger target = fxArgToByteOffset(the, 1, 0);
	txInteger offset = view->value.dataView.offset + (target * delta);	
	if (array->next && (array->next->kind == XS_TYPED_ARRAY_KIND)) {
		txSlot* arrayDispatch = array->next;
		txSlot* arrayView = arrayDispatch->next;
		txSlot* arrayData = arrayView->next->value.reference->next;
		txU2 shift = arrayDispatch->value.typedArray.dispatch->shift;
		txInteger arrayLength = arrayView->value.dataView.size >> shift;
		txInteger arrayOffset = arrayView->value.dataView.offset;	
		txInteger limit = offset + (arrayLength * delta);
		if ((target < 0) || (length - arrayLength < target))
			mxRangeError("invalid offset");
		if (data == arrayData) {
			txSlot* resultData;
			mxPush(mxArrayBufferConstructor);
			mxNew();
			mxPushInteger(arrayLength << shift);
			mxRunCount(1);
			resultData = the->stack->value.reference->next;
			c_memcpy(resultData->value.arrayBuffer.address, arrayData->value.arrayBuffer.address + arrayOffset, arrayLength << shift);
			arrayData = resultData;
			arrayOffset = 0;
		}
		else 
			mxPushUndefined();
		if (arrayData->value.arrayBuffer.address == C_NULL)
			mxTypeError("detached buffer");
		if (data->value.arrayBuffer.address == C_NULL)
			mxTypeError("detached buffer");
		if (dispatch == arrayDispatch) {
			c_memcpy(data->value.arrayBuffer.address + offset, arrayData->value.arrayBuffer.address + arrayOffset, limit - offset);
		}
		else {
			txInteger arrayDelta = 1 << shift;
			mxPushUndefined();
			while (offset < limit) {
				(*arrayDispatch->value.typedArray.dispatch->getter)(the, arrayData, arrayOffset, the->stack, EndianNative);
				(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
				(*dispatch->value.typedArray.dispatch->setter)(the, data, offset, the->stack, EndianNative);
				arrayOffset += arrayDelta;
				offset += delta;
			}
			the->stack++;
		}
		the->stack++;
	}
	else {
		txInteger count, index;
		mxPushSlot(mxArgv(0));
		fxGetID(the, mxID(_length));
		count = fxToInteger(the, the->stack);
		mxPop();
		if ((target < 0) || (length - count < target))
			mxRangeError("invalid offset");
		index = 0;
		while (index < count) {
			mxPushSlot(mxArgv(0));
			fxGetID(the, index);
			if (data->value.arrayBuffer.address == C_NULL)
				mxTypeError("detached buffer");
			(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
			(*dispatch->value.typedArray.dispatch->setter)(the, data, offset, the->stack, EndianNative);
			mxPop();
			offset += delta;
			index++;
		}	
	}
}

void fx_TypedArray_prototype_slice(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger start = (txInteger)fxArgToIndex(the, 0, 0, length);
	txInteger end = (txInteger)fxArgToIndex(the, 1, length, length);
	txInteger count = (end > start) ? end - start : 0;
	txInteger index = 0;
	fxCreateTypedArraySpecies(the);
	mxPushNumber(count);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		mxResultTypedArrayDeclarations;
		if (resultLength < count)
			mxTypeError("insufficient buffer");
		if (count) {
			if (data->value.arrayBuffer.address == C_NULL)
				mxTypeError("detached buffer");
			mxPushUndefined();
			while (start < end) {
				(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + (start * delta), the->stack, EndianNative);
				(*resultDispatch->value.typedArray.dispatch->coerce)(the, the->stack);
				(*resultDispatch->value.typedArray.dispatch->setter)(the, resultData, resultView->value.dataView.offset + (index << resultDispatch->value.typedArray.dispatch->shift), the->stack, EndianNative);
				start++;
				index++;
			}
			mxPop();
		}
	}
}

void fx_TypedArray_prototype_some(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
		mxResult->value.boolean = fxToBoolean(the, the->stack++);
		if (mxResult->value.boolean)
			break;
		index++;
	}
}

void fx_TypedArray_prototype_sort(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txSlot* function = C_NULL;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind != XS_UNDEFINED_KIND) {
			if (fxIsCallable(the, slot))
				function = slot;
			else
				mxTypeError("compare is no function");
		}
	}
	if (function) {
		/* like GCC qsort */
		#define COMPARE(INDEX) \
			fxCompareTypedArrayItem(the, function, dispatch, view, data, INDEX)
		#define MOVE(FROM,TO) \
			from = data->value.arrayBuffer.address + view->value.dataView.offset + ((FROM) * delta); \
			to = data->value.arrayBuffer.address + view->value.dataView.offset + ((TO) * delta); \
			for (k = 0; k < delta; k++) *to++ = *from++
		#define PUSH(INDEX) \
			mxPushUndefined(); \
			(*dispatch->value.typedArray.dispatch->getter)(the, data, view->value.dataView.offset + ((INDEX) * delta), the->stack, EndianNative)
		#define PULL(INDEX) \
			(*dispatch->value.typedArray.dispatch->setter)(the, data, view->value.dataView.offset + ((INDEX) * delta), the->stack++, EndianNative)
		if (length > 0) {
			txInteger i, j, k;
			txByte* from;
			txByte* to;
			if (length > mxSortThreshold) {
				txInteger lo = 0, hi = length - 1;
				txSortPartition stack[mxSortStackSize];
				txSortPartition *top = stack + 1;
				while (stack < top) {
					txIndex mid = lo + ((hi - lo) >> 1);
					PUSH(mid);
					if (COMPARE(lo) > 0) {
						MOVE(lo, mid);
						PULL(lo);
						PUSH(mid);
					}
					if (COMPARE(hi) < 0) {
						MOVE(hi, mid);
						PULL(hi);
						PUSH(mid);
						if (COMPARE(lo) > 0) {
							MOVE(lo, mid);
							PULL(lo);
							PUSH(mid);
						}
					}
					i = lo + 1;
					j = hi - 1;
					do {
						while (COMPARE(i) < 0) i++;
						while (COMPARE(j) > 0) j--;
						if (i < j) {
							PUSH(i);
							MOVE(j, i);
							PULL(j);
							i++;
							j--;
						}
						else if (i == j) {
							i++;
							j--;
							break;
						}
					} while (i <= j);
					if ((j - lo) <= mxSortThreshold) {
						if ((hi - i) <= mxSortThreshold) {
							top--;
							lo = top->lo; 
							hi = top->hi;
						}
						else {
							lo = i;
						}
					}
					else if ((hi - i) <= mxSortThreshold) {
						hi = j;
					}
					else if ((j - lo) > (hi - i)) {
						top->lo = lo;
						top->hi = j; 
						top++;
						lo = i;
					}
					else {
						top->lo = i;
						top->hi = hi; 
						top++;
						hi = j;
					}
				}
			}
			for (i = 1; i < length; i++) {
				PUSH(i);
				for (j = i; (j > 0) && (COMPARE(j - 1) > 0); j--) {
					MOVE(j - 1, j);
				}	
				PULL(j);
			}
		}
	}
	else
		c_qsort(data->value.arrayBuffer.address, length, delta, dispatch->value.typedArray.dispatch->compare);
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_subarray(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txU2 shift = dispatch->value.typedArray.dispatch->shift;
	txInteger length = view->value.dataView.size >> shift;
	txInteger start = (txInteger)fxArgToIndex(the, 0, 0, length);
	txInteger stop = (txInteger)fxArgToIndex(the, 1, length, length);
	if (stop < start) 
		stop = start;
	fxCheckArrayBufferDetached(the, buffer, XS_IMMUTABLE);
	fxCreateTypedArraySpecies(the);
	mxPushSlot(buffer);
	mxPushInteger(view->value.dataView.offset + (start << shift));
	mxPushInteger(stop - start);
	mxRunCount(3);
	mxPullSlot(mxResult);
	fxCheckTypedArrayInstance(the, mxResult);
}

void fx_TypedArray_prototype_toLocaleString(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txInteger length = view->value.dataView.size / delta;
	txInteger index = 0;
	txString string;
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txBoolean comma = 0;
	txInteger size = 0;
	mxPushStringX(",");
	the->stack->kind += XS_KEY_KIND - XS_STRING_KIND;
	the->stack->value.key.sum = 1;
	while (index < length) {
		if (comma) {
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			size += slot->value.key.sum;
		}
		else
			comma = 1;
		mxPushSlot(mxThis);
		fxGetIndex(the, index);
		if ((the->stack->kind != XS_UNDEFINED_KIND) && (the->stack->kind != XS_NULL_KIND)) {
			mxDub();
			fxGetID(the, mxID(_toLocaleString));
			mxCall();
			mxRunCount(0);
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			string = fxToString(the, slot);
			slot->kind += XS_KEY_KIND - XS_STRING_KIND;
			slot->value.key.sum = c_strlen(string);
			size += slot->value.key.sum;
		}
		mxPop();
		index++;
	}
	string = mxResult->value.string = fxNewChunk(the, size + 1);
	slot = list->next;
	while (slot) {
		c_memcpy(string, slot->value.key.string, slot->value.key.sum);
		string += slot->value.key.sum;
		slot = slot->next;
	}
	*string = 0;
	mxResult->kind = XS_STRING_KIND;
	mxPop();
}

void fx_TypedArray_prototype_toStringTag_get(txMachine* the)
{
	if (mxThis->kind == XS_REFERENCE_KIND) {
        txSlot* instance = mxThis->value.reference;
        txSlot* slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_TYPED_ARRAY_KIND)) {
			txTypeDispatch *dispatch = instance->next->value.typedArray.dispatch;
			txSlot* key = fxGetKey(the, mxID(dispatch->constructorID));
			if (key->kind == XS_KEY_X_KIND)
				mxResult->kind = XS_STRING_X_KIND;
			else
				mxResult->kind = XS_STRING_KIND;
			mxResult->value.string = key->value.key.string;
		}
	}
}

void fx_TypedArray_prototype_values(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* property;
	fxCheckArrayBufferDetached(the, instance->next->next->next, XS_IMMUTABLE);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis));
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	mxPullSlot(mxResult);
}

#if mxBigEndian
	#define mxEndianDouble_BtoN(a) (a)
	#define mxEndianFloat_BtoN(a) (a)
	#define mxEndianS64_BtoN(a) (a)
	#define mxEndianU64_BtoN(a) (a)
	#define mxEndianS32_BtoN(a) (a)
	#define mxEndianU32_BtoN(a) (a)
	#define mxEndianS16_BtoN(a) (a)
	#define mxEndianU16_BtoN(a) (a)

	#define mxEndianDouble_NtoB(a) (a)
	#define mxEndianFloat_NtoB(a) (a)
	#define mxEndianS64_NtoB(a) (a)
	#define mxEndianU64_NtoB(a) (a)
	#define mxEndianS32_NtoB(a) (a)
	#define mxEndianU32_NtoB(a) (a)
	#define mxEndianS16_NtoB(a) (a)
	#define mxEndianU16_NtoB(a) (a)
#else
	#define mxEndianDouble_LtoN(a) (a)
	#define mxEndianFloat_LtoN(a) (a)
	#define mxEndianS64_LtoN(a) (a)
	#define mxEndianU64_LtoN(a) (a)
	#define mxEndianS32_LtoN(a) (a)
	#define mxEndianU32_LtoN(a) (a)
	#define mxEndianS16_LtoN(a) (a)
	#define mxEndianU16_LtoN(a) (a)

	#define mxEndianDouble_NtoL(a) (a)
	#define mxEndianFloat_NtoL(a) (a)
	#define mxEndianS64_NtoL(a) (a)
	#define mxEndianU64_NtoL(a) (a)
	#define mxEndianS32_NtoL(a) (a)
	#define mxEndianU32_NtoL(a) (a)
	#define mxEndianS16_NtoL(a) (a)
	#define mxEndianU16_NtoL(a) (a)
#endif

#if mxLittleEndian
	#define mxEndianDouble_BtoN(a) (mxEndianDouble_Swap(a))
	#define mxEndianFloat_BtoN(a) (mxEndianFloat_Swap(a))
	#define mxEndianS64_BtoN(a) ((txS8) mxEndian64_Swap(a))
	#define mxEndianU64_BtoN(a) ((txU8) mxEndian64_Swap(a))
	#define mxEndianS32_BtoN(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_BtoN(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_BtoN(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_BtoN(a) ((txU2) mxEndian16_Swap(a))

	#define mxEndianDouble_NtoB(a) (mxEndianDouble_Swap(a))
	#define mxEndianFloat_NtoB(a) (mxEndianFloat_Swap(a))
	#define mxEndianS64_NtoB(a) ((txS8) mxEndian64_Swap(a))
	#define mxEndianU64_NtoB(a) ((txU8) mxEndian64_Swap(a))
	#define mxEndianS32_NtoB(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_NtoB(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_NtoB(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_NtoB(a) ((txU2) mxEndian16_Swap(a))
#else
	#define mxEndianDouble_LtoN(a) (mxEndianDouble_Swap(a))
	#define mxEndianFloat_LtoN(a) (mxEndianFloat_Swap(a))
	#define mxEndianS64_LtoN(a) ((txS8) mxEndian64_Swap(a))
	#define mxEndianU64_LtoN(a) ((txU8) mxEndian64_Swap(a))
	#define mxEndianS32_LtoN(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_LtoN(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_LtoN(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_LtoN(a) ((txU2) mxEndian16_Swap(a))

	#define mxEndianDouble_NtoL(a) (mxEndianDouble_Swap(a))
	#define mxEndianFloat_NtoL(a) (mxEndianFloat_Swap(a))
	#define mxEndianS64_NtoL(a) ((txS8) mxEndian64_Swap(a))
	#define mxEndianU64_NtoL(a) ((txU8) mxEndian64_Swap(a))
	#define mxEndianS32_NtoL(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_NtoL(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_NtoL(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_NtoL(a) ((txU2) mxEndian16_Swap(a))
#endif

#if defined(__GNUC__) || defined(__llvm__)
	#define mxEndian16_Swap(a) __builtin_bswap16(a)
#else
	static txU2 mxEndian16_Swap(txU2 a)
	{
		txU2 b;
		txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
		int i;
		for (i = 0; i < 2; i++)
			p2[i] = p1[1 - i];
		return b;
	}
#endif

#if defined(__GNUC__) || defined(__llvm__)
	#define mxEndian32_Swap(a) __builtin_bswap32(a)
#else
	static txU4 mxEndian32_Swap(txU4 a)
	{
		txU4 b;
		txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
		int i;
		for (i = 0; i < 4; i++)
			p2[i] = p1[3 - i];
		return b;
	}
#endif

#if defined(__GNUC__) || defined(__llvm__)
	#define mxEndian64_Swap(a) __builtin_bswap64(a)
#else
	static txU8 mxEndian64_Swap(txU8 a)
	{
		txU4 b;
		txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
		int i;
		for (i = 0; i < 8; i++)
			p2[i] = p1[7 - i];
		return b;
	}
#endif

static float mxEndianFloat_Swap(float a)
{
#if defined(__GNUC__) || defined(__llvm__)
	uint32_t result = __builtin_bswap32(*(uint32_t *)&a);
	return *(float *)&result;
#else
	float b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 4; i++)
		p2[i] = p1[3 - i];
	return b;
#endif
}

static double mxEndianDouble_Swap(double a)
{
#if defined(__GNUC__) || defined(__llvm__)
	uint64_t result = __builtin_bswap64(*(uint64_t *)&a);
	return *(double *)&result;
#else
	double b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 8; i++)
		p2[i] = p1[7 - i];
	return b;
#endif
}

#define toNative(size, endian) mxEndian##size##_##endian##toN
#define fromNative(size, endian) mxEndian##size##_Nto##endian
#define IMPORT(size) (endian == EndianBig ? toNative(size, B)(value) : endian == EndianLittle ? toNative(size, L)(value) : (value))
#define EXPORT(size) (endian == EndianBig ? fromNative(size, B)(value) : endian == EndianLittle ? toNative(size, L)(value) : (value))

int fxBigInt64Compare(const void* p, const void* q)
{
	txS8 a = *((txS8*)p);
	txS8 b = *((txS8*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxBigInt64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS8 value;
#ifdef mxMisalignedSettersCrash
	value = c_read32(data->value.arrayBuffer.address + offset);
#else
	value = *((txS8*)(data->value.arrayBuffer.address + offset));
#endif
	value = IMPORT(S64);
	fxFromBigInt64(the, slot, value);
}

void fxBigInt64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS8 value = (txS8)fxToBigInt64(the, slot);
#ifdef mxMisalignedSettersCrash
	value = EXPORT(S64);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txS8));
#else
	*((txS8*)(data->value.arrayBuffer.address + offset)) = EXPORT(S64);
#endif
}

int fxBigUint64Compare(const void* p, const void* q)
{
	txU8 a = *((txU8*)p);
	txU8 b = *((txU8*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxBigUint64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU8 value;
#ifdef mxMisalignedSettersCrash
	value = c_read32(data->value.arrayBuffer.address + offset);
#else
	value = *((txU8*)(data->value.arrayBuffer.address + offset));
#endif
	value = IMPORT(U64);
	fxFromBigUint64(the, slot, value);
}

void fxBigUint64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU8 value = (txU8)fxToBigUint64(the, slot);
#ifdef mxMisalignedSettersCrash
	value = EXPORT(U64);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txU8));
#else
	*((txU8*)(data->value.arrayBuffer.address + offset)) = EXPORT(U64);
#endif
}

int fxFloat32Compare(const void* p, const void* q)
{
	float a = *((float*)p);
	float b = *((float*)q);
	if (c_isnan(a)) {
		if (c_isnan(b)) 
			return 0;
		return 1;
	}
	if (c_isnan(b))
		return -1;
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxFloat32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	float value;
	slot->kind = XS_NUMBER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((float*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.number = IMPORT(Float);
}

void fxFloat32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	float value = (float)slot->value.number;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(Float);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(float));
#else
	*((float*)(data->value.arrayBuffer.address + offset)) = EXPORT(Float);
#endif
}

int fxFloat64Compare(const void* p, const void* q)
{
	double a = *((double*)p);
	double b = *((double*)q);
	if (c_isnan(a)) {
		if (c_isnan(b)) 
			return 0;
		return 1;
	}
	if (c_isnan(b))
		return -1;
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxFloat64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	double value;
	slot->kind = XS_NUMBER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((double*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.number = IMPORT(Double);
}

void fxFloat64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	double value = slot->value.number;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(Double);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(double));
#else
	*((double*)(data->value.arrayBuffer.address + offset)) = EXPORT(Double);
#endif
}

void fxIntCoerce(txMachine* the, txSlot* slot)
{
	fxToInteger(the, slot);
}

void fxUintCoerce(txMachine* the, txSlot* slot)
{
	fxToUnsigned(the, slot);
}

int fxInt8Compare(const void* p, const void* q)
{
	txS1 a = *((txS1*)p);
	txS1 b = *((txS1*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	slot->kind = XS_INTEGER_KIND;
	slot->value.integer = *((txS1*)(data->value.arrayBuffer.address + offset));
}

void fxInt8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	*((txS1*)(data->value.arrayBuffer.address + offset)) = (txS1)slot->value.integer;
}

int fxInt16Compare(const void* p, const void* q)
{
	txS2 a = *((txS2*)p);
	txS2 b = *((txS2*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS2 value;
	slot->kind = XS_INTEGER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((txS2*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.integer = IMPORT(S16);
}

void fxInt16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS2 value = (txS2)slot->value.integer;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(S16);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txS2));
#else
	*((txS2*)(data->value.arrayBuffer.address + offset)) = EXPORT(S16);
#endif
}

int fxInt32Compare(const void* p, const void* q)
{
	txS4 a = *((txS4*)p);
	txS4 b = *((txS4*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS4 value;
	slot->kind = XS_INTEGER_KIND;
#ifdef mxMisalignedSettersCrash
	value = c_read32(data->value.arrayBuffer.address + offset);
#else
	value = *((txS4*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.integer = IMPORT(S32);
}

void fxInt32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS4 value = (txS4)slot->value.integer;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(S32);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txS4));
#else
	*((txS4*)(data->value.arrayBuffer.address + offset)) = EXPORT(S32);
#endif
}

int fxUint8Compare(const void* p, const void* q)
{
	txU1 a = c_read8((txU1*)p);
	txU1 b = c_read8((txU1*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	slot->kind = XS_INTEGER_KIND;
	slot->value.integer = c_read8((txU1*)(data->value.arrayBuffer.address + offset));
}

void fxUint8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txUnsigned tmp = (slot->kind == XS_INTEGER_KIND) ? (txUnsigned)slot->value.integer : (txUnsigned)slot->value.number;
	*((txU1*)(data->value.arrayBuffer.address + offset)) = (txU1)tmp;
}

int fxUint16Compare(const void* p, const void* q)
{
	txU2 a = *((txU2*)p);
	txU2 b = *((txU2*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU2 value;
	slot->kind = XS_INTEGER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((txU2*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.integer = IMPORT(U16);
}

void fxUint16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txUnsigned tmp = (slot->kind == XS_INTEGER_KIND) ? (txUnsigned)slot->value.integer : (txUnsigned)slot->value.number;
	txU2 value = (txU2)tmp;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(U16);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txU2));
#else
	*((txU2*)(data->value.arrayBuffer.address + offset)) = EXPORT(U16);
#endif
}

int fxUint32Compare(const void* p, const void* q)
{
	txU4 a = *((txU4*)p);
	txU4 b = *((txU4*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
#ifdef mxMisalignedSettersCrash
	txUnsigned value = c_read32(data->value.arrayBuffer.address + offset);
#else
	txUnsigned value = *((txU4*)(data->value.arrayBuffer.address + offset));
#endif
	value = IMPORT(U32);
	if (((txInteger)value) >= 0) {
		slot->kind = XS_INTEGER_KIND;
		slot->value.integer = value;
	}
	else {
		slot->kind = XS_NUMBER_KIND;
		slot->value.number = value;
	}
}

void fxUint32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU4 value = (slot->kind == XS_INTEGER_KIND) ? (txU4)slot->value.integer : (txU4)slot->value.number;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(U32);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txU4));
#else
	*((txU4*)(data->value.arrayBuffer.address + offset)) = EXPORT(U32);
#endif
}

void fxUint8ClampedSetter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txNumber value = fxToNumber(the, slot);
	if (value <= 0)
		value = 0;
	else if (value >= 255)
		value = 255;
	else if (c_isnan(value))
		value = 0;
	else
		value = c_nearbyint(value);
	*((txU1*)(data->value.arrayBuffer.address + offset)) = (txU1)value;
}

