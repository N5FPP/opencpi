/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <sys/time.h>
#include <strings.h>
#include <climits>
#include "OcpiUtilException.h"
#include "OcpiUtilProtocol.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilMisc.h"

namespace OCPI {
  namespace OA = OCPI::API;
  namespace OE = OCPI::Util::EzXml;
  namespace Util {

    Operation::Operation()
      : m_isTwoWay(false), m_nArgs(0), m_args(NULL), m_nExceptions(0), m_exceptions(NULL),
	m_myOffset(0), m_topFixedSequence(false) {
    }
    Operation::~Operation() {
      if (m_args)
	delete [] m_args;
      if (m_exceptions)
	delete [] m_exceptions;
    }
    Operation & 
    Operation::
    operator=(const Operation & p ) {
      return operator=(&p);
    }
    
    Operation & 
    Operation::
    operator=(const Operation * p )
    {
      m_name = p->m_name;
      m_qualifiedName = p->m_qualifiedName;
      m_isTwoWay = p->m_isTwoWay;
      m_nArgs = p->m_nArgs;
      m_args = m_nArgs ? new Member[ m_nArgs ] : NULL;
      for (unsigned n = 0; n < m_nArgs; n++)
	m_args[n] = p->m_args[n];
      m_nExceptions = p->m_nExceptions;
      m_exceptions = m_nExceptions ? new Operation[m_nExceptions] : NULL;
      for (unsigned n = 0; n < m_nExceptions; n++)
	m_exceptions[n] = p->m_exceptions[n];
      m_myOffset = p->m_myOffset;
      m_topFixedSequence = p->m_topFixedSequence;
      return *this;
    }
    const char *Operation::parse(ezxml_t op, Protocol &p) {
      const char *err;
      if ((err = OE::checkAttrs(op, "Name", "Twoway", "QualifiedName", (void*)0)))
	return err;
      const char *name = ezxml_cattr(op, "Name");
      if (!name)
	return "Missing \"Name\" attribute for operation";
      for (Operation *o = p.m_operations; o < this; o++)
	if (!strcasecmp(name, o->m_name.c_str()))
	  return esprintf("Duplicate operation name \"%s\" in protocol \"%s\"",
			  name, p.m_name.c_str());
      m_name = name;
      name = ezxml_cattr(op, "qualifiedname");
      if (name)
	m_qualifiedName = name;
      bool sub32dummy;
      size_t maxAlignDummy = 0;
      if ((err = OE::getBoolean(op, "TwoWay", &m_isTwoWay)))
	return err;
      if (m_isTwoWay)
	p.m_isTwoWay = true;
      if (!(err = Member::parseMembers(op, m_nArgs, m_args, false, "argument", NULL))) {
	if (m_nArgs == 1 &&
	    ((m_args[0].isSequence() && m_args[0].isFixed()) ||
	     (!m_args[0].isSequence() && m_args[0].m_baseType == OA::OCPI_String)))
	  m_topFixedSequence = true;
	err = Member::alignMembers(m_args, m_nArgs, maxAlignDummy, m_myOffset,
				   p.m_dataValueWidth, p.m_diverseDataSizes,
				   sub32dummy, p.m_isUnbounded, p.m_variableMessageLength,
				   m_topFixedSequence);
      }
      return err;
    }
    Member *Operation::findArg(const char *name) const {
      Member *a = m_args;
      for (unsigned n = 0; n < m_nArgs; n++, a++)
	if (!strcasecmp(name, a->m_name.c_str()))
	  return a;
      return NULL;
    }

    size_t Operation::
    defaultLength() const {
      size_t length = m_myOffset;
      if (args()) {
	Member &m = args()[nArgs()-1];
	if (m.m_isSequence)
	  if (nArgs() == 1)
	    length = 0;
	  else
	    length = m.m_offset + m.m_align;
	else if (!m.m_arrayRank && m.m_baseType == OA::OCPI_String)
	  length = m.m_offset + 1;
      }
      return length;
    }
    Operation *Protocol::findOperation(const char *name) {
      Operation *o = m_operations;
      for (unsigned n = 0; n < m_nOperations; n++, o++)
	if (!strcasecmp(name, o->m_name.c_str()))
	  return o;
      return NULL;
    }

    void Operation::printXML(std::string &out, unsigned indent) const {
      formatAdd(out, "%*s<operation", indent * 2, "");
      if (!m_name.empty())
	formatAdd(out, " name=\"%s\"", m_name.c_str());
      if (!m_qualifiedName.empty())
	formatAdd(out, " qualifiedName=\"%s\"", m_qualifiedName.c_str());
      if (m_nArgs) {
	formatAdd(out, ">\n");
	Member *a = m_args;
	for (unsigned n = 0; n < m_nArgs; n++, a++)
	  a->printXML(out, "argument", indent + 1);
	formatAdd(out, "%*s</operation>\n", indent * 2, "");
      } else
	formatAdd(out, "/>\n");
    }
    void Operation::write(Writer &writer, const uint8_t *data, size_t length) {
      for (size_t n = 0; n < m_nArgs; n++)
	m_args[n].write(writer, data, length, isTopFixedSequence());
    }

    size_t Operation::read(Reader &reader, uint8_t *data, size_t maxLength) {
      size_t max = maxLength;
      bool fake = data == NULL;
      for (unsigned n = 0; n < m_nArgs; n++)
	m_args[n].read(reader, data, maxLength, fake);
      return max - maxLength;
    }

    void Operation::generate(const char *name, Protocol &p) {
      m_name = name;
      m_nArgs = random() % 10;
      Member *m = m_args = m_nArgs ? new Member[m_nArgs] : NULL;
      for (unsigned n = 0; n < m_nArgs; n++, m++) {
	char *aname;
	ocpiCheck(asprintf(&aname, "arg%d", n) > 0);
	m->generate(aname);
	free(aname);
      }
      const char *err;
      bool sub32dummy = false;
      size_t maxAlignDummy = 1;
      if ((err = Member::alignMembers(m_args, m_nArgs, maxAlignDummy, m_myOffset,
				      p.m_dataValueWidth, p.m_diverseDataSizes,
				      sub32dummy, p.m_isUnbounded, p.m_variableMessageLength,
				      m_topFixedSequence)))
	throw std::string(err);

    }
    void Operation::generateArgs(Value **&v) {
      v = m_nArgs ? new Value *[m_nArgs] : 0;
      for (unsigned n = 0; n < m_nArgs; n++) {
	v[n] = new Value(m_args[n], NULL);
	v[n]->generate();
      }
    }
    void Operation::print(FILE *f, Value **v) const {
      fprintf(f, "%s\n", m_name.c_str());
      for (unsigned n = 0; n < m_nArgs; n++) {
	std::string s;
	v[n]->unparse(s);
	fprintf(f, "  %u: %s\n", n, s.c_str());
      }
    }
    void Operation::testPrintParse(FILE *f, Value **v, bool hex) {
      for (unsigned n = 0; n < m_nArgs; n++) {
	std::string s;
	fprintf(f, "%s: ", m_args[n].m_name.c_str());
	v[n]->unparse(s, NULL, false, hex);
	Value tv(m_args[n]);
	const char *err;
	if ((err = tv.parse(s.c_str(), NULL))) {
	  fprintf(f, "error: %s (%s)\n", err, s.c_str());
	  exit(1);
	} else {
	  fprintf(f, "parse ok\n");
	  std::string s1;
	  tv.unparse(s1, NULL, false, hex);
	  if (s != s1) {
	    fprintf(f,
		    "\n"
		    "error: mismatch0: ***%s***\n"
		    "error: mismatch1: ***%s***\n",
		    s.c_str(), s1.c_str());
	    exit(1);
	  }
	}
      }
      fprintf(f, "\n");
      
    }

    void Protocol::init() {
      m_nOperations = 0;
      m_operations = NULL;
      m_op = NULL;
      m_defaultBufferSize = SIZE_MAX;
      m_minBufferSize = 0;
      m_dataValueWidth = 8;
      m_dataValueGranularity = 1;
      m_diverseDataSizes = false;
      m_minMessageValues = 0;
      m_maxMessageValues = SIZE_MAX; // default is no maximum.
      m_variableMessageLength = false;
      m_zeroLengthMessages = false;
      m_isTwoWay = false;
      m_isUnbounded = false;
    }
    // These defaults are for when there is no protocol at all.
    // Notice that the default is for 1 byte fixed size messages...
    Protocol::Protocol() {
      init();
    }
    Protocol::
    Protocol(const Protocol & p )
    {
      *this = p;
    }
    // morph/clone constructor where we can take from source
    Protocol::
    Protocol(Protocol *p) {
      if (p) {
	m_nOperations = p->m_nOperations;
	m_operations = p->m_operations;
	p->m_operations = NULL;
	m_op = NULL;
	m_qualifiedName = p->m_qualifiedName;
	m_file = p->m_file;
	m_name = p->m_name;
	m_defaultBufferSize = p->m_defaultBufferSize;
	m_minBufferSize = p->m_minBufferSize;
	m_dataValueWidth = p->m_dataValueWidth;
	m_dataValueGranularity = p->m_dataValueGranularity;
	m_diverseDataSizes = p->m_diverseDataSizes;
	m_minMessageValues = p->m_minMessageValues;
	m_maxMessageValues = p->m_maxMessageValues;
	m_variableMessageLength = p->m_variableMessageLength;
	m_zeroLengthMessages = p->m_zeroLengthMessages;
	m_isTwoWay = p->m_isTwoWay;
	m_isUnbounded = p->m_isUnbounded;
      } else
	init();
    }
    Protocol::~Protocol() {

      if (m_operations)
	delete [] m_operations;
    }
    Protocol & 
    Protocol::
    operator=( const Protocol & p ) 
    {
      return operator=(&p);
    }
    // FIXME: This would all be unnecessary if we interned protocols..
    Protocol & 
    Protocol::
    operator=( const Protocol * p )
    {
      
      m_nOperations = p->m_nOperations;
      m_operations = m_nOperations ? new Operation[m_nOperations] : NULL;
      for (unsigned int n = 0; n < m_nOperations; n++)
	m_operations[n] = p->m_operations[n];
      m_op = NULL;
      m_qualifiedName = p->m_qualifiedName;
      m_file = p->m_file;
      m_name = p->m_name;
      m_defaultBufferSize = p->m_defaultBufferSize;
      m_minBufferSize = p->m_minBufferSize;
      m_dataValueWidth = p->m_dataValueWidth;
      m_dataValueGranularity = p->m_dataValueGranularity;
      m_diverseDataSizes = p->m_diverseDataSizes;
      m_minMessageValues = p->m_minMessageValues;
      m_maxMessageValues = p->m_maxMessageValues;
      m_variableMessageLength = p->m_variableMessageLength;
      m_zeroLengthMessages = p->m_zeroLengthMessages;
      m_isTwoWay = p->m_isTwoWay;
      m_isUnbounded = p->m_isUnbounded;
      return *this;
    }

    void Protocol::
    finishOperation(const Operation &op) {
      if (m_isUnbounded ||
	  (m_maxMessageValues != SIZE_MAX && m_maxMessageValues != op.m_myOffset))
	m_variableMessageLength = true; // see below for more setting of this
      if (m_maxMessageValues == SIZE_MAX || op.m_myOffset > m_maxMessageValues)
	m_maxMessageValues = op.m_myOffset; // still in bytes until later
      size_t minLength; // smallest message possible for this operation
      if (op.m_nArgs) {
	Member &m = op.m_args[op.m_nArgs - 1];
	minLength =
	  m.m_isSequence ? (op.m_nArgs == 1 ? 0 : m.m_offset + sizeof(uint32_t)) : op.m_myOffset;
      } else
	minLength = 0;
      if (minLength > m_minBufferSize)
	m_minBufferSize = minLength; // the maximum of the smallest per operation
      if (minLength < m_minMessageValues)
	m_minMessageValues = minLength; // the minimum of the smallest per operation
      if (minLength == 0)
	m_zeroLengthMessages = true;
      if (op.m_myOffset < m_minMessageValues)
	m_minMessageValues = op.m_myOffset;
      // Find smallest element for data granularity purposes
      // When a more disruptive change is required, this could be done in 
      // alignMembers and offset()
      size_t smallest = SIZE_MAX;
      for (unsigned n = 0; n < op.m_nArgs; n++) {
	Member &m = op.m_args[n];
	if (m.m_elementBytes && m.m_elementBytes < smallest)
	  smallest = m.m_elementBytes;
      }
      if (smallest != SIZE_MAX) {
	smallest *= CHAR_BIT;
	assert(smallest % m_dataValueWidth == 0);
	smallest /= m_dataValueWidth;
	if (smallest < m_dataValueGranularity)
	  m_dataValueGranularity = smallest;
      }
    }

    const char *Protocol::
    parseOperation(ezxml_t op) {
      const char *err;
      if ((err = OE::checkTag(op, "operation", "within protocol \"%s\"",
			      m_name.c_str())))
	return err;
      const char *name = ezxml_name(op);
      // FIXME:  support xi:included protocols
      if (!name || strcasecmp(name, "Operation"))
	return "Element under Protocol is neither Operation, Protocol or xi:include";
      // If this is NULL we're just counting properties.
      if (!m_operations) {
	m_nOperations++;
	return NULL;
      }
      Operation *o = m_op++;
      if (!(err = o->parse(op, *this)))
	finishOperation(*o);
      return err;
    }
    // Interface for C iterator in ezxml
    static const char *doOperation(ezxml_t op, void *arg) {
      return ((Protocol *)arg)->parseOperation(op);
    }
    // Parse summary attributes, presumably when there is no explicit protocol
    // although possibly to forcibly override, so don't "set default".
    // This is called in these contexts:
    // In an explicit child element in the spec
    // In the port element in a spec
    // in the port element in an impl
    const char *Protocol::parseSummary(ezxml_t pSum) {
      bool maxSet = false, minSet = false, ddsSet = false, zlmSet = false, vlmSet = false,
	unBoundedSet = false, twoWaySet = false;
      const char *err;
      if ((err = OE::getNumber(pSum, "DataValueWidth", &m_dataValueWidth, NULL, 0, false)) ||
          (err = OE::getNumber(pSum, "DataValueGranularity", &m_dataValueGranularity, NULL, 0,
			       false)) ||
          (err = OE::getBoolean(pSum, "DiverseDataSizes", &m_diverseDataSizes, false, false,
				&ddsSet)) ||
          (err = OE::getBoolean(pSum, "ZeroLengthMessages", &m_zeroLengthMessages, false,
				false, &zlmSet)) ||
          (err = OE::getBoolean(pSum, "VariableMessageLength", &m_variableMessageLength, false,
				false, &vlmSet)) ||
	  (err = OE::getBoolean(pSum, "unBounded", &m_isUnbounded, false, false,
				&unBoundedSet)) ||
	  (err = OE::getBoolean(pSum, "twoway", &m_isTwoWay, false, false, &twoWaySet)) ||
	  (err = OE::getNumber(pSum, "MaxMessageValues", &m_maxMessageValues, &maxSet, 0,
			       false)) ||
          (err = OE::getNumber(pSum, "MinMessageValues", &m_minMessageValues, &minSet, 0,
			       false)))
	return err;
      if (maxSet || minSet || ddsSet || zlmSet || vlmSet || unBoundedSet || twoWaySet) {
	// specifying ZLM also overrides minMessageValues
	if (!minSet && m_zeroLengthMessages)
	  m_minMessageValues = 0;
	// Fill in any complicated defaults
	if (m_zeroLengthMessages && m_minMessageValues != 0)
	  return "MinMessageValues cannot > 0 when ZeroLengthMessages is true";
	if (!maxSet) {
	  m_variableMessageLength = true;
	  m_isUnbounded = true;
	}
      }
      return NULL;
    }

    const char *Protocol::finishParse() {
      m_minBufferSize = (m_dataValueWidth * m_minMessageValues + 7) / 8;
      return NULL;
    }

    void Protocol::generate(const char *name) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      srandom((unsigned)(tv.tv_sec + tv.tv_usec));
      m_name = name;
      m_dataValueWidth = 0;
      m_nOperations = random() % 10 + 1;
      Operation *o = m_operations = new Operation[m_nOperations];
      for (unsigned n = 0; n < m_nOperations; n++, o++) {
	char *opName;
	ocpiCheck(asprintf(&opName, "op%d", n) > 0);
	o->generate(opName, *this);
	free(opName);
	finishOperation(*o);
      }
      finishParse();
    }
    // Generate a message for a random opcode
    void Protocol::generateOperation(uint8_t &opcode, Value **&v) {
      opcode = (uint8_t)(random() % m_nOperations);
      m_operations[opcode].generateArgs(v);
    }

    void Protocol::printOperation(FILE *f, uint8_t opcode, Value **v) const {
      fprintf(f, "%u:", opcode);
      m_operations[opcode].print(f, v);
      fflush(f);
    }
    void Protocol::testOperation(FILE *f, uint8_t opcode, Value **v, bool hex) {
      fprintf(f, "testing %u:", opcode);
      m_operations[opcode].testPrintParse(f, v, hex);
    }
    const char *Protocol::parse(char *proto) {
      ezxml_t x;
      const char *err = OE::ezxml_parse_str(proto, strlen(proto), x);
      if (err ||
	  (err = parse(x, NULL, NULL, doOperation, this)))
	err = esprintf("Error parsing xml protocol description: %s", err);
      return err;
    }

    // Top level protocol parsing method.
    const char *Protocol::parse(ezxml_t prot, const char *defName, const char *file,
				const char *(*doChild)(ezxml_t child, void *arg), void *arg) {
      if (file)
	m_file = file;
      const char *name = ezxml_cattr(prot, "name");
      m_name = name ? name : (defName ? defName : "");
      if ((name = ezxml_cattr(prot, "qualifiedname")))
	m_qualifiedName = name;
      m_dataValueWidth = 0;
      const char *err;
      if ((err = OE::checkAttrs(prot, "Name", "QualifiedName", "defaultbuffersize",
				"datavaluewidth", "maxmessagevalues", "minmessagevalues",
				"datavaluegranularity", "diversedatasizes", "unbounded",
				"zerolengthmessages", "variablemessagelength", (void*)0)) ||
	  (err = OE::checkElements(prot, "operation", "xi:include", (void*)0)) ||
	  (err = OE::ezxml_children(prot, doChild ? doChild : doOperation,
				    arg ? arg : this)))
	return err;
      if (m_nOperations) {
	m_operations = m_op = new Operation[m_nOperations];
	// Now we call a second time t0 make them.
	size_t save = m_dataValueGranularity;
	m_dataValueGranularity = SIZE_MAX;
	if ((err = OE::ezxml_children(prot, doChild ? doChild : doOperation,
				      arg ? arg : this)) ||
	    (err = OE::getBoolean(prot, "ZeroLengthMessages", &m_zeroLengthMessages, true)))
	  return err;
	if (m_dataValueGranularity == SIZE_MAX)
	  m_dataValueGranularity = save;
	// Allow dvw to be overridden to provide for future finer granularity 
	// (e.g. force 8 when proto says 16)
	size_t dvwattr;
	bool hasDvw;
	if ((err = OE::getNumber(prot, "datavaluewidth", &dvwattr, &hasDvw, m_dataValueWidth)) ||
	    (err = OE::getNumber(prot, "datavaluegranularity", &m_dataValueGranularity, NULL,
				 m_dataValueGranularity)))
	  return err;
	if (hasDvw) {
	  if (dvwattr > m_dataValueWidth)
	    return "can't force DataValueWidth to be greater than protocol implies";
	  else if (m_dataValueWidth % dvwattr)
	    return "DataValueWidth attribute must divide into implied datavaluewidth";
	  m_dataValueWidth = dvwattr;
	}
	if (m_dataValueWidth && m_maxMessageValues != SIZE_MAX) {
	  // Convert max size from bytes back to values
	  size_t bytes = (m_dataValueWidth + CHAR_BIT - 1) / CHAR_BIT;
	  m_maxMessageValues += bytes - 1;
	  m_maxMessageValues /= bytes;
	}
	// Now we can still override the real max message values
	if ((err = OE::getNumber(prot, "maxmessagevalues", &m_maxMessageValues, 0, 0, false)))
	  return err;
	// We can also override the granularity
	
      }
      // Note we parse the summary attribute to allow overriding of attributes derived from
      // the operations.
      if ((err = parseSummary(prot)))
	return err;
      if ((err = OE::getNumber(prot, "defaultbuffersize", &m_defaultBufferSize, 0,
			       m_isUnbounded || m_maxMessageValues == SIZE_MAX ?
			       SIZE_MAX : (m_maxMessageValues * m_dataValueWidth + 7) / 8)))
	return err;
      return finishParse();
    }
    void Protocol::printXML(std::string &out, unsigned indent) const {
      formatAdd(out, "%*s<protocol", indent * 2, "");
      if (!m_name.empty())
	formatAdd(out, " name=\"%s\"", m_name.c_str());
      if (!m_qualifiedName.empty())
	formatAdd(out, " qualifiedName=\"%s\"", m_qualifiedName.c_str());
      if (m_defaultBufferSize != SIZE_MAX)
	formatAdd(out, " defaultbuffersize=\"%zu\"", m_defaultBufferSize);
      // We emit all the summary attributes that MIGHT have overridden the protocol
      // If we kept track of what was overridden we could prune this...
      formatAdd(out,
		" dataValueWidth=\"%zu\""
		" dataValueGranularity=\"%zu\""
		" diverseDataSizes=\"%u\""
		" minMessageValues=\"%zu\"",
		m_dataValueWidth, m_dataValueGranularity, m_diverseDataSizes,
		m_minMessageValues);
      if (m_maxMessageValues != SIZE_MAX)
	formatAdd(out, " maxMessageValues=\"%zu\"", m_maxMessageValues);
      if (m_diverseDataSizes)
	formatAdd(out, " diversedatasizes=\"true\"");
      if (m_zeroLengthMessages)
	formatAdd(out, " zeroLengthMessages=\"true\"");
      if (m_variableMessageLength)
	formatAdd(out, " variableMessageLength=\"true\"");
      if (m_isTwoWay)
	formatAdd(out, " twoWay=\"true\"");
      if (m_isUnbounded)
	formatAdd(out, " unBounded=\"true\"");
      if (m_operations) {
	formatAdd(out, ">\n");
	Operation *o = m_operations;
	for (unsigned n = 0; n < m_nOperations; n++, o++)
	  o->printXML(out, indent + 1);
	formatAdd(out, "%*s</protocol>\n", indent * 2, "");
      } else
	formatAdd(out, "/>\n");
    }
    // Send the data in the buffer to the writer
    void Protocol::write(Writer &writer, const uint8_t *data, size_t length, uint8_t opcode) {
      assert(!((intptr_t)data & (maxDataTypeAlignment - 1)));
      if (!m_operations)
	throw Error("No operations in protocol for writing");
      if (opcode >= m_nOperations)
	throw Error("Invalid Opcode for protocol");
      writer.writeOpcode(m_operations[opcode].m_name.c_str(), opcode);
      m_operations[opcode].write(writer, data, length);
      writer.end();
    }
    size_t Protocol::read(Reader &reader, uint8_t *data, size_t maxLength, uint8_t opcode) {
      assert(!((intptr_t)data & (maxDataTypeAlignment - 1)));
      if (!m_operations)
	throw Error("No operations in protocol for reading");
      if (opcode >= m_nOperations)
	throw Error("Invalid Opcode for protocol");
      size_t size = m_operations[opcode].read(reader, data, maxLength);
      reader.end();
      return size;
    }
  }
}
