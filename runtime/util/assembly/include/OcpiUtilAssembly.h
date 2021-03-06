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

/*
 * Definitions for assembly metadata decoding, etc.
 * The result of parsing an xml assembly, in a vacuum (no access to impl metadata)
 *
 * After parsing this class does not depend on the existence of the xml from which it
 * was parsed.
 *
 * Example:
 * <assembly name="myapp">
 *  <instance name="fred" specName="fft1d"/>
 *  <instance name="i2" specName="sink" selection="select-expression">
 *    <property name="knob1" value="4"/>
 *  </instance>
 *  <connection name=helpin>
 *    <port name="input" instance="fred"/>
 *    <external name="globin" provider="true" url="ddstopic:"/>
 *  </connection>
 * </assembly>
 */
#ifndef OCPI_UTIL_ASSEMBLY_H
#define OCPI_UTIL_ASSEMBLY_H
#include <string>
#include <vector>
#include <list>
#include "ezxml.h"
#include "OcpiPValue.h"

namespace OCPI {
  namespace Util {
    // This class is the bottom of a three-level stack of assembly handling.
    // It is parsed and maintained in a "vacuum": i.e. it knows nothing about
    // implementations (metadata), nothing about librarys (available implementations),
    // and nothing about containers (available places to run).  It simply represents
    // the parsed entity.  See the "library" facility for the next level up,
    // which is where implementation awareness (from artifacts in libraries) is handled.
    class Assembly {
    public:
      // This class is overloaded both for property values for individual instances
      // as well as top level properties that are mapped to workers.
      struct Instance;
      struct MappedProperty {
        std::string m_name;
        std::string m_instPropName; // non-empty for top level
        unsigned m_instance;        // if m_instPropName is nonempty this is valid
        const char *parse(ezxml_t x, Assembly &a, const PValue *params);
      };
      typedef std::vector<MappedProperty> MappedProperties;
      typedef uint32_t Delay;
      struct Property {
        std::string m_name;
        bool m_hasValue; // since value might legitimately be an empty string
        std::string m_value;
        std::string m_dumpFile;
        bool m_hasDelay;
        Delay m_delay;
        Property() : m_hasValue(false), m_hasDelay(false), m_delay(0) {}
        const char *parse(ezxml_t x, Property *first = NULL);
        const char *setValue(ezxml_t px);
        void setValue(const char *name, const char *value);
      };
      typedef std::vector<Property> Properties;
      struct Port;
      // Capture the values that drive processor allocation policy.
      #define COLLOCATION_POLICY_ATTRS \
        "minCollocation", "maxCollocation", "minContainers", "maxContainers"
      struct CollocationPolicy {
        size_t m_minCollocation, m_maxCollocation, m_minContainers, m_maxContainers;
        CollocationPolicy();
        const char
          *parse(ezxml_t x),
          *apply(size_t scale, size_t nContainers,
                 size_t &collocation, size_t &usedContainers, size_t &finalScale)
          const;
      };
      struct Instance {
	std::string
	  m_name,                  // name of the instance within the assembly
	  m_specName,              // name of component or worker being instantiated
	  m_implName,              // name of implementation (may be a path)
	  m_selection;             // the selection expression
	unsigned m_ordinal;
	bool     m_externals;      // whether all ports should be considered external
	std::vector<unsigned> m_slaves;
	unsigned m_master;
	bool     m_hasMaster;
	Properties m_properties;
	PValueList m_parameters;
	std::list<Port*> m_ports; // attachments to connections
	typedef std::list<Port*>::iterator PortsIter;
	CollocationPolicy m_collocation;
	ezxml_t m_xml;
	bool m_freeXml;
	Instance();
	~Instance();
	const char *cname() const { return m_name.c_str(); }
	const char
	  *parse(ezxml_t ix, Assembly &a, unsigned ordinal, const char **extraInstAttrs,
		 const PValue *params),
	  *addProperty(const char *name, ezxml_t px),
	  *parseConnection(ezxml_t ix, Assembly &a, const PValue *params),
	  *checkSlave(Assembly &a, const char *name),
	  *setProperty(const char *propAssign);
	ezxml_t xml() const { return m_xml; }
	const std::vector<unsigned> &slaves() const { return m_slaves; }
      };
      struct Role {
        bool m_knownRole;     // role is known
        bool m_bidirectional; // possible when inherited from a port
        bool m_provider;      // is this attachment acting as a provider to the world?
        Role();
      };
      // The attachment of a connection to external or port
      struct External {
        std::string m_name;   // the name of the "external port" to the assembly
        std::string m_url;    // the URL that this external attachment has
        Role m_role;
        size_t m_index;       // This is only used for top level "external" element
        size_t m_count;
        PValueList m_parameters;
        External();
        const char *parse(ezxml_t, const char *, unsigned&, const PValue *pvl);
        const char *init(const char *name, const char *role = NULL);
      };
      typedef std::list<External> Externals;
      typedef Externals::iterator ExternalsIter;
      struct Connection;
      struct Port {
        // This mutable is because this name might be resolved when an application
        // uses this assembly (and has access to impl metadata).
        // Then this assembly is reused, this resolution will still be valid.
        mutable std::string m_name;
        mutable Role m_role;
        unsigned m_instance;
        size_t m_index;
        // This mutable is because some port parameter values are added later by name
        // and the XML assembly might not use port names
        mutable PValueList m_parameters;
        Port *m_connectedPort; // the "other" port of the connection
        const char *cname() const { return m_name.c_str(); }
        const char *parse(ezxml_t x, Assembly &a, const PValue *pvl, const PValue *params);
        const char *init(Assembly &a, const char *name, unsigned instance, bool isInput,
                         bool bidi, bool known, size_t index, const PValue *params);
      };
      struct Connection {
        std::string m_name;
        Externals m_externals;
        std::list<Port> m_ports;
        typedef std::list<Port>::iterator PortsIter;
        PValueList m_parameters;
        size_t m_count; // all attachments have same count. zero if unknown
        Connection();
        const char *parse(ezxml_t x, Assembly &a, unsigned &ord, const OCPI::Util::PValue *params);
        const char *addPort(Assembly &a, unsigned instance, const char *port, bool isInput,
                            bool bidi, bool known, size_t index,
                            const OCPI::Util::PValue *params, Port *&);
        External &addExternal();
      };
      // Potentially specified in the assembly, what policy should be used
      // to spread workers to containers?
      enum CMapPolicy {
        RoundRobin,
        MinProcessors,
        MaxProcessors
      };
    private:
      ezxml_t m_xml;
      char *m_copy;
      bool m_xmlOnly;
      bool m_isImpl; // Is this assembly of worker (implementation) instances or component instances?
      const char *parse(const char *defaultName = NULL, const char **extraTopAttrs = NULL,
                        const char **extraInstAttrs = NULL, const OCPI::Util::PValue *params = NULL);
      std::vector<Instance*> m_instances;
    public:
      Instance &utilInstance(size_t n) { return *m_instances[n]; }
      size_t nUtilInstances() { return m_instances.size(); }
      const std::string &name() const { return m_name; }
      static unsigned s_count;
      std::string m_name;
      std::string m_package;
      int m_doneInstance; // -1 for none
      std::list<Connection> m_connections;
      typedef std::list<Connection>::iterator ConnectionsIter;
      CMapPolicy m_cMapPolicy;
      size_t   m_processors;
      MappedProperties m_mappedProperties; // top level mapped to instance properties.
      CollocationPolicy m_collocation;
      // Provide a file name.
      explicit Assembly(const char *file, const char **extraTopAttrs = NULL,
                        const char **extraInstAttrs = NULL, const OCPI::Util::PValue *params = NULL);
      // Provide a string containing the xml
      explicit Assembly(const std::string &string, const char **extraTopAttrs = NULL,
                        const char **extraInstAttrs = NULL, const OCPI::Util::PValue *params = NULL);
      // Provide XML directly
      explicit Assembly(const ezxml_t top, const char *defaultName, bool isImpl,
                        const char **topAttrs = NULL, const char **instAttrs = NULL,
                        const OCPI::Util::PValue *params = NULL);
      ~Assembly();
      const char
        *addInstance(ezxml_t ix, const char **extraInstAttrs, const PValue *params,
                     bool addXml = false),
        *findInstanceForParam(const char *pName, const char *&assign, unsigned &instn),
        *checkInstanceParams(const char *pName, const PValue *params, bool checkMapped = false,
                             bool singleAssignment = false),
        *addConnection(const char *name, Connection *&c),
        *getInstance(const char *name, unsigned &),
        *addPortConnection(unsigned from, const char *name, unsigned to, const char *toPort,
                           const char *transport, const OCPI::Util::PValue *params),
        *addExternalConnection(unsigned instance, const char *port,
                               const OCPI::Util::PValue *params = NULL, bool isInput = false,
                               bool bidi = false, bool known = false),
        *addExternalConnection(ezxml_t x, const OCPI::Util::PValue *params);
      inline ezxml_t xml() { return m_xml; }
      inline bool isImpl() { return m_isImpl; }
    };
  }
}
#endif
