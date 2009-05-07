// Copyright (c) 2008  INRIA Sophia-Antipolis (France), ETHZ (Suisse).
// Copyrigth (c) 2009  GeometryFactory (France)
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s) : Camille Wormser, Pierre Alliez, Laurent Rineau, Stephane Tayeb

#ifndef CGAL_AABB_TREE_H
#define CGAL_AABB_TREE_H

#include <vector>
#include <iterator>
#include <CGAL/AABB_node.h>
#include <CGAL/AABB_search_tree.h>

namespace CGAL {

    /**
    * @class AABB_tree
    *
    *
    */
    template <typename AABBTraits>
    class AABB_tree
    {
    public:
        /// types
        typedef typename AABBTraits::FT FT;
        typedef typename AABBTraits::Point Point;
        typedef typename AABBTraits::Primitive Primitive;
        typedef typename AABBTraits::Size_type Size_type; 
        typedef typename AABBTraits::Bounding_box Bounding_box;
        typedef typename AABBTraits::Point_and_primitive Point_and_primitive;
        typedef typename AABBTraits::Object_and_primitive Object_and_primitive;

    private:
        // internal KD-tree used to accelerate the distance queries
        typedef AABB_search_tree<AABBTraits> Search_tree;

    public:
        /**
        * @brief Constructor
        * @param first iterator over first primitive to insert
        * @param beyond past-the-end iterator
        *
        * Builds the datastructure. Type ConstPrimitiveIterator can be any const
        * iterator on a container of Primitive::id_type such that Primitive has
        * a constructor taking a ConstPrimitiveIterator as argument.
        */
        template<typename ConstPrimitiveIterator>
        AABB_tree(ConstPrimitiveIterator first, ConstPrimitiveIterator beyond);

        /// Clears the current tree and rebuilds the datastructure.
        /// Type ConstPrimitiveIterator can be any const iterator on
        /// a container of Primitive::id_type such that Primitive has
        /// a constructor taking a ConstPrimitiveIterator as argument.
        /// Returns true if the memory allocation was successful.
        template<typename ConstPrimitiveIterator>
        bool rebuild(ConstPrimitiveIterator first, ConstPrimitiveIterator beyond);

        /// Non virtual destructor
        ~AABB_tree()
        { 
            clear(); 
         }

        /// Clears the tree
        void clear()
        {
            // clear AABB tree
            m_data.clear();
            delete [] m_p_root;
            m_p_root = NULL;

            clear_search_tree();
        }


        // bbox and size
        Bounding_box bbox() const { return m_p_root->bbox(); }
        Size_type size() const { return m_data.size(); }
        bool empty() const { return m_data.empty(); }

        /// Construct internal search tree with a given point set
        // returns true iff successful memory allocation
        template<typename ConstPointIterator>
        bool accelerate_distance_queries(ConstPointIterator first, ConstPointIterator beyond);

        /// Construct internal search tree from
        /// a point set taken on the internal primitives
        // returns true iff successful memory allocation
        bool accelerate_distance_queries();

        template<typename Query>
        bool do_intersect(const Query& query) const;

        template<typename Query>
        Size_type number_of_intersected_primitives(const Query& query) const;

        // all intersections
        template<typename Query, typename OutputIterator>
        OutputIterator all_intersected_primitives(const Query& query, OutputIterator out) const;
        template<typename Query, typename OutputIterator>
        OutputIterator all_intersections(const Query& query, OutputIterator out) const;

        // any intersection
        template <typename Query>
        boost::optional<Primitive> any_intersected_primitive(const Query& query) const;
        template <typename Query>
        boost::optional<Object_and_primitive> any_intersection(const Query& query) const;

        // distance queries
        FT squared_distance(const Point& query) const;
        FT squared_distance(const Point& query, const Point& hint) const;
        Point closest_point(const Point& query);
        Point closest_point(const Point& query, const Point& hint);
        Primitive closest_primitive(const Point& query) const;
        Primitive closest_primitive(const Point& query, const Point& hint) const;
        Point_and_primitive closest_point_and_primitive(const Point& query) const;
        Point_and_primitive closest_point_and_primitive(const Point& query, const Point& hint) const;

    private:

        // clears internal KD tree
        void clear_search_tree()
        {
            delete m_p_search_tree;
            m_p_search_tree = NULL;
            m_search_tree_constructed = false;
        }


        /// generic traversal of the tree
        template <class Query, class Traversal_traits>
        void traversal(const Query& query, Traversal_traits& traits) const
        {
            if(!empty())
                m_p_root->template traversal<Traversal_traits,Query>(query, traits, m_data.size());
            else
                std::cerr << "AABB tree traversal with empty tree" << std::endl;
        }
        //////////////////////////////////////////////

    private:
        typedef AABB_node<AABBTraits> Node;
        typedef typename AABBTraits::Sphere Sphere;

        //-------------------------------------------------------
        // Traits classes for traversal computation
        //-------------------------------------------------------
        /**
        * @class First_intersection_traits
        */
        template<typename Query>
        class First_intersection_traits
        {
        public:
            typedef typename boost::optional<Object_and_primitive> Result;
        public:
            First_intersection_traits()
                : m_is_found(false)
            {}

            bool go_further() const { return !m_is_found; }

            void intersection(const Query& query, const Primitive& primitive)
            {
                Object_and_primitive op;
                m_is_found = AABBTraits().intersection(query, primitive, op);
                if(m_is_found)
                    m_result = Result(op);
            }

            bool do_intersect(const Query& query, const Node& node) const
            {
                return AABBTraits().do_intersect(query, node.bbox());
            }

            Result result() const { return m_result; }
            bool is_intersection_found() const { return m_is_found; }

        private:
            bool m_is_found;
            Result m_result;
        };


        /**
        * @class Counting_traits
        */
        template<typename Query>
        class Counting_traits
        {
        public:
            Counting_traits()
                : m_nb_intersections(0)
            {}

            bool go_further() const { return true; }

            // TOCHECK
            void intersection(const Query& query, const Primitive& primitive)
            {
                if( AABBTraits().do_intersect(query, primitive) )
                {
                    ++m_nb_intersections;
                }
            }

            bool do_intersect(const Query& query, const Node& node) const
            {
                return AABBTraits().do_intersect(query, node.bbox());
            }

            Size_type number_of_intersections() const { return m_nb_intersections; }

        private:
            Size_type m_nb_intersections;
        };


        /**
        * @class Listing_intersection_traits
        */
        template<typename Query, typename Output_iterator>
        class Listing_intersection_traits
        {
        public:
            Listing_intersection_traits(Output_iterator out_it)
                : m_intersection()
                , m_out_it(out_it) {}

            bool go_further() const { return true; }

            void intersection(const Query& query, const Primitive& primitive)
            {
                Object_and_primitive intersection;
                if( AABBTraits().intersection(query, primitive, intersection) )
                {
                    *m_out_it++ = intersection;
                }
            }

            bool do_intersect(const Query& query, const Node& node) const
            {
                return AABBTraits().do_intersect(query, node.bbox());
            }

        private:
            Output_iterator m_out_it;
        };


        /**
        * @class Listing_primitive_traits
        */
        template<typename Query, typename Output_iterator>
        class Listing_primitive_traits
        {
        public:
            Listing_primitive_traits(Output_iterator out_it)
                : m_out_it(out_it) {}

            bool go_further() const { return true; }

            void intersection(const Query& query, const Primitive& primitive)
            {
                if( AABBTraits().do_intersect(query, primitive) )
                {
                    *m_out_it++ = primitive;
                }
            }

            bool do_intersect(const Query& query, const Node& node) const
            {
                return AABBTraits().do_intersect(query, node.bbox());
            }

        private:
            Output_iterator m_out_it;
        };


        /**
        * @class First_primitive_traits
        */
        template<typename Query>
        class First_primitive_traits
        {
        public:
            First_primitive_traits()
                : m_is_found(false)
                , m_result() {}

            bool go_further() const { return !m_is_found; }

            void intersection(const Query& query, const Primitive& primitive)
            {
                if( AABBTraits().do_intersect(query, primitive) )
                {
                    m_result = boost::optional<Primitive>(primitive);
                    m_is_found = true;
                }
            }

            bool do_intersect(const Query& query, const Node& node) const
            {
                return AABBTraits().do_intersect(query, node.bbox());
            }

            boost::optional<Primitive> result() const { return m_result; }
            bool is_intersection_found() const { return m_is_found; }

        private:
            bool m_is_found;
            boost::optional<Primitive> m_result;
        };

        /**
        * @class Projection_traits
        */
        class Distance_traits
        {
        public:
            Distance_traits(const Point& query,
                            const Point& hint,
                            const Primitive& hint_primitive)
                            : m_closest_point(hint),
                              m_closest_primitive(hint_primitive),
                              m_sphere(AABBTraits().sphere(query,hint))
            {}

            bool go_further() const { return true; }

            void intersection(const Point& query, const Primitive& primitive)
            {
                // TOFIX
                // update m_closest_primitive if needed
                Point new_closest_point = 
                    AABBTraits().closest_point(query, primitive, m_closest_point);
                if(new_closest_point != m_closest_point)
                {
                    m_closest_primitive = primitive;
                    m_closest_point = new_closest_point;
                }
                m_sphere = AABBTraits().sphere(query, m_closest_point);
            }

            bool do_intersect(const Point& query, const Node& node) const
            {
                return AABBTraits().do_intersect(m_sphere, node.bbox());
            }

            Point closest_point() const { return m_closest_point; }
            Primitive closest_primitive() const { return m_closest_primitive; }

        private:
            Sphere m_sphere;
            Point m_closest_point;
            Primitive m_closest_primitive;
        };

    private:
        // returns a point guaranteed to be on one primitive
        Point any_reference_point()
        {
            CGAL_assertion(!empty());
            return m_data[0].reference_point();
        }

    public:
        Point best_hint(const Point& query)
        {
            if(m_search_tree_constructed)
                return m_p_search_tree->closest_point(query); 
            else 
                return this->any_reference_point();
        }

    private:
        // set of input primitives
        std::vector<Primitive> m_data;
        // single root node
        Node* m_p_root;
        // search KD-tree
        Search_tree* m_p_search_tree;
        bool m_search_tree_constructed;

    private:
        // Disabled copy constructor & assignment operator
        typedef AABB_tree<AABBTraits> Self;
        AABB_tree(const Self& src);
        Self& operator=(const Self& src);

    };  // end class AABB_tree

    template<typename Tr>
    template<typename ConstPrimitiveIterator>
    AABB_tree<Tr>::AABB_tree(ConstPrimitiveIterator first,
        ConstPrimitiveIterator beyond)
        : m_data()
        , m_p_root(NULL)
        , m_p_search_tree(NULL)
        , m_search_tree_constructed(false)
    {
        // Insert each primitive into tree
        // TODO: get number of elements to reserve space ?
        while ( first != beyond )
        {
            m_data.push_back(Primitive(first));
            ++first;
        }

        m_p_root = new Node[m_data.size()-1]();
        if(m_p_root == NULL)
        {
            std::cerr << "Unable to allocate memory for AABB tree" << std::endl;
            CGAL_assertion(m_p_root != NULL);
            m_data.clear();
        }
        else
            m_p_root->expand(m_data.begin(), m_data.end(), m_data.size());
    }

    // Clears tree and insert a set of primitives
    // Returns true upon successful memory allocation
    template<typename Tr>
    template<typename ConstPrimitiveIterator>
    bool AABB_tree<Tr>::rebuild(ConstPrimitiveIterator first,
                                ConstPrimitiveIterator beyond)
    {
        // cleanup current tree and internal KD tree
        clear();

        // inserts primitives
        while(first != beyond)
        {
            m_data.push_back(Primitive(first));
            first++;
        }

        // allocates tree nodes
        m_p_root = new Node[m_data.size()-1]();
        if(m_p_root == NULL)
        {
            std::cerr << "Unable to allocate memory for AABB tree" << std::endl;
            m_data.clear();
            return false;
        }

        // constructs the tree
        m_p_root->expand(m_data.begin(), m_data.end(), m_data.size());
        return true;
    }


    // constructs the search KD tree from given points
    // to accelerate the distance queries
    template<typename Tr>
    template<typename ConstPointIterator>
    bool AABB_tree<Tr>::accelerate_distance_queries(ConstPointIterator first,
                                                    ConstPointIterator beyond)
    {
        // clears current KD tree
        clear_search_tree();
        
        m_p_search_tree = new Search_tree(first, beyond);
        if(m_p_search_tree != NULL)
        {
            m_search_tree_constructed = true;
            return true;
        }
        else
            return false;
    }

    // constructs the search KD tree from interal primitives
    template<typename Tr>
    bool AABB_tree<Tr>::accelerate_distance_queries()
    {
        CGAL_assertion(!m_data.empty());

        // iterate over primitives to get reference points on them
        std::vector<Point> points;
        typename std::vector<Primitive>::const_iterator it;
        for(it = m_data.begin(); it != m_data.end(); ++it)
            points.push_back(it->reference_point());

        return accelerate_distance_queries(points.begin(), points.end());
    }

    template<typename Tr>
    template<typename Query>
    bool
        AABB_tree<Tr>::do_intersect(const Query& query) const
    {
        First_intersection_traits<Query> traversal_traits;
        this->traversal(query, traversal_traits);
        return traversal_traits.is_intersection_found();
    }

    template<typename Tr>
    template<typename Query>
    typename Tr::Size_type
        AABB_tree<Tr>::number_of_intersected_primitives(const Query& query) const
    {
        Counting_traits<Query> traversal_traits;
        this->traversal(query, traversal_traits);
        return traversal_traits.number_of_intersections();
    }

    template<typename Tr>
    template<typename Query, typename OutputIterator>
    OutputIterator
        AABB_tree<Tr>::all_intersected_primitives(const Query& query,
                                                  OutputIterator out) const
    {
        Listing_primitive_traits<Query, OutputIterator> traversal_traits(out);
        this->traversal(query, traversal_traits);
        return out;
    }

    template<typename Tr>
    template<typename Query, typename OutputIterator>
    OutputIterator
        AABB_tree<Tr>::all_intersections(const Query& query,
                                         OutputIterator out) const
    {
        Listing_intersection_traits<Query, OutputIterator> traversal_traits(out);
        this->traversal(query, traversal_traits);
        return out;
    }

    template <typename Tr>
    template <typename Query>
    boost::optional<typename Tr::Object_and_primitive>
        AABB_tree<Tr>::any_intersection(const Query& query) const
    {
        First_intersection_traits<Query> traversal_traits;
        this->traversal(query, traversal_traits);
        return traversal_traits.result();
    }

    template <typename Tr>
    template <typename Query>
    boost::optional<typename Tr::Primitive>
        AABB_tree<Tr>::any_intersected_primitive(const Query& query) const
    {
        First_primitive_traits<Query> traversal_traits;
        this->traversal(query, traversal_traits);
        return traversal_traits.result();
    }

    // closest point with user-specified hint
    template<typename Tr>
    typename AABB_tree<Tr>::Point
        AABB_tree<Tr>::closest_point(const Point& query,
                                     const Point& hint)
    {
        Primitive hint_primitive = m_data[0];
        Distance_traits distance_traits(query,hint,hint_primitive);
        this->traversal(query, distance_traits);
        return distance_traits.closest_point();
    }

    // closest point without hint, the search KD-tree is queried for the
    // first closest neighbor point to get a hint
    template<typename Tr>
    typename AABB_tree<Tr>::Point
        AABB_tree<Tr>::closest_point(const Point& query)
    {
        const Point hint = best_hint(query);
        return closest_point(query,hint);
    }

    // squared distance with user-specified hint
    template<typename Tr>
    typename AABB_tree<Tr>::FT
        AABB_tree<Tr>::squared_distance(const Point& query,
                                        const Point& hint) const
    {
        const Point closest = this->closest_point(query, hint);
        return CGAL::squared_distance(query, closest);
    }

    // squared distance without user-specified hint
    template<typename Tr>
    typename AABB_tree<Tr>::FT
        AABB_tree<Tr>::squared_distance(const Point& query) const
    {
        const Point closest = this->closest_point(query);
        return CGAL::squared_distance(query, closest);
    }

    // closest point with user-specified hint
    template<typename Tr>
    typename AABB_tree<Tr>::Primitive
        AABB_tree<Tr>::closest_primitive(const Point& query) const
    {
        return closest_primitive(query,best_hint(query));
    }

    // closest point with user-specified hint
    template<typename Tr>
    typename AABB_tree<Tr>::Primitive
        AABB_tree<Tr>::closest_primitive(const Point& query,
                                         const Point& hint) const
    {
        const Point hint = best_hint(query);
        Distance_traits distance_traits(query,hint,any_primitive());
        this->traversal(query, distance_traits);
        return distance_traits.closest_primitive();
    }

} // end namespace CGAL

#endif // CGAL_AABB_TREE_H
