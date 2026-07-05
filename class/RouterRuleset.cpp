#include "RouterRuleset.hpp"
#include "ABackend.hpp"
#include <string>
#include <vector>

std::vector<
    std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > > >::const_iterator
RouterRuleset::get_corresponding_router(std::string const &URI) const
{
    std::vector<std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > > >::
        const_iterator ite = backend_rules.end();

    for (std::vector<std::pair<
             std::string, std::pair<t_backend_type, std::vector<std::string> > > >::const_iterator
             it = backend_rules.begin();
         it != ite; ++it)
    {
        if (it->first.size() <= URI.size() &&
            !URI.compare(URI.size() - it->first.size(), it->first.size(), it->first))
            return (it);
    }
    return (backend_rules.end());
}
