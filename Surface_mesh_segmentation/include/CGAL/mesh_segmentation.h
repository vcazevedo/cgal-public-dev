#ifndef CGAL_SURFACE_MESH_SEGMENTATION_MESH_SEGMENTATION_H
#define CGAL_SURFACE_MESH_SEGMENTATION_MESH_SEGMENTATION_H

/** 
 * @file mesh_segmentation.h
 * @brief The API which contains free template functions for SDF computation and mesh segmentation.
 */
#include <CGAL/internal/Surface_mesh_segmentation/Surface_mesh_segmentation.h>
#include <boost/config.hpp>

namespace CGAL {

  
 /// @cond SKIP_IN_MANUAL
template <bool Fast_sdf_calculation_mode, class Polyhedron, class SDFPropertyMap, class GeomTraits
#ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
  = typename Polyhedron::Traits
#endif
>
std::pair<double, double> 
compute_sdf_values( const Polyhedron& polyhedron, 
                    SDFPropertyMap sdf_values, 
                    double cone_angle = 2.0 / 3.0 * CGAL_PI,
                    int number_of_rays = 25,
                    bool postprocess = true,
                    GeomTraits traits = GeomTraits())
{
    internal::Surface_mesh_segmentation<Polyhedron, GeomTraits, Fast_sdf_calculation_mode> algorithm(polyhedron, traits);
    return algorithm.calculate_sdf_values(cone_angle, number_of_rays, sdf_values, postprocess);
}
/// @endcond

/*!
 * \ingroup PkgSurfaceSegmentation
 * @brief Function computing the Shape Diameter Function over a surface mesh.
 *
 * This function implements the Shape Diameter Function (SDF) as described in \cgalCite{shapira2008consistent}.
 * It is possible to compute raw SDF values (without post-processing). In such a case,
 * -1 is used to indicate when no SDF value could be computed for a facet.
 * 
 * @pre @a polyhedron.is_pure_triangle()
 *
 * @tparam Polyhedron a %CGAL polyhedron
 * @tparam SDFPropertyMap  a `ReadWritePropertyMap` with `Polyhedron::Facet_const_handle` as key and `double` as value type
 * @tparam GeomTraits a model of SegmentationGeomTraits
 *
 * @param polyhedron surface mesh on which SDF values are computed
 * @param[out] sdf_values the SDF value of each facet
 * @param cone_angle opening angle in radians for the cone of each facet
 * @param number_of_rays number of rays picked in the cone of each facet. In our experiments, we observe that increasing the number of rays beyond the default has a little effect on the quality of the segmentation result
 * @param postprocess if `true`, `CGAL::postprocess_sdf_values` is called on raw SDF value computed.
 * @param traits traits class
 *
 * @return minimum and maximum raw SDF values if @a postprocess is `true`, otherwise minimum and maximum SDF values (before linear normalization)
 */
template <class Polyhedron, class SDFPropertyMap, class GeomTraits
#ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
  = typename Polyhedron::Traits
#endif
>
std::pair<double, double>
compute_sdf_values( const Polyhedron& polyhedron, 
                    SDFPropertyMap sdf_values, 
                    double cone_angle = 2.0 / 3.0 * CGAL_PI,
                    int number_of_rays = 25,
                    bool postprocess = true,
                    GeomTraits traits = GeomTraits())
{
    return compute_sdf_values<true, Polyhedron, SDFPropertyMap, GeomTraits>
      (polyhedron, sdf_values, cone_angle, number_of_rays, postprocess, traits);
}


/*!
 * \ingroup PkgSurfaceSegmentation
 * @brief Function post-processing raw SDF values computed per facet.
 *
 * Post-processing steps applied :
 *   - Facets with -1 SDF values are assigned the average SDF value of their edge-adjacent neighbors. 
 *     If there is still a facet having -1 SDF value, the minimum valid SDF value assigned to it. Note that this step is not inherited from the paper. 
 *     The main reason for not assigning 0 to facets with no SDF values (i.e. -1) is that it can obstruct log-normalization process which takes place at the beginning of `CGAL::segment_from_sdf_values`.
 *   - SDF values are smoothed with bilateral filtering.
 *   - SDF values are linearly normalized between [0,1].
 *
 * See the section \ref Surface_mesh_segmentationPostprocessing for more details.
 * 
 * @pre @a polyhedron.is_pure_triangle()
 * @pre Raw values should be greater or equal to 0. -1 indicates when no value could be computed
 *
 * @tparam Polyhedron a %CGAL polyhedron
 * @tparam SDFPropertyMap  a `ReadWritePropertyMap` with `Polyhedron::Facet_const_handle` as key and `double` as value type
 *
 * @param polyhedron surface mesh on which SDF values are computed
 * @param[in, out] sdf_values the SDF value of each facet
 *
 * @return minimum and maximum SDF values before linear normalization
 */
template<class Polyhedron, class SDFPropertyMap>
std::pair<double, double> 
postprocess_sdf_values(const Polyhedron& polyhedron, SDFPropertyMap sdf_values) {
    CGAL_precondition(polyhedron.is_pure_triangle());
    return internal::Postprocess_sdf_values<Polyhedron>().postprocess(polyhedron, sdf_values);
}


/*!
 * \ingroup PkgSurfaceSegmentation
 * @brief Function computing the segmentation of a surface mesh given an SDF value per facet. 
 *
 * This function fills a property map which associates a segment-id (in [0, number of segments -1]) 
 * or a cluster-id (in [0, `number_of_clusters` -1]) to each facet.
 * A segment is a set of connected facets which are placed under the same cluster (see \cgalFigureRef{Cluster_vs_segment}).
 *
 * \note Log-normalization is applied on `sdf_values` before segmentation. 
 *       As described in the original paper \cgalCite{shapira2008consistent},
 *       this normalization is done to preserve thin parts of the mesh
 *       by increasing the distance between smaller SDF values and reducing
 *       it between larger ones.
 * \note There is no direct relation between the parameter `number_of_clusters`
 * and the final number of segments after segmentation. However, setting a large number of clusters will result in a detailed segmentation of the mesh with a large number of segments.
 *
 * @pre @a polyhedron.is_pure_triangle()
 * @pre @a number_of_clusters > 0
 *
 * @tparam Polyhedron a %CGAL polyhedron
 * @tparam SDFPropertyMap  a `ReadablePropertyMap` with `Polyhedron::Facet_const_handle` as key and `double` as value type
 * @tparam SegmentPropertyMap a `ReadWritePropertyMap` with `Polyhedron::Facet_const_handle` as key and `int` as value type
 * @tparam GeomTraits a model of SegmentationGeomTraits
 *
 * @param polyhedron surface mesh corresponding to the SDF values
 * @param sdf_values the SDF value of each facet between [0-1]
 * @param[out] segment_ids the segment or cluster id of each facet
 * @param number_of_clusters number of clusters for the soft clustering
 * @param smoothing_lambda factor which indicates the importance of the surface features for the energy minimization. It is recommended to choose a value in the interval [0,1]. See the section \ref Surface_mesh_segmentationGraphCut for more details.
 * @param extract_segments if `true` fill `segment_ids` with segment-ids and cluster-ids otherwise (see \cgalFigureRef{Cluster_vs_segment})
 * @param traits traits class
 *
 * @return number of segments if `extract_segments` is set to `true` and `number_of_clusters` otherwise
 */ 
template <class Polyhedron, class SDFPropertyMap, class SegmentPropertyMap, class GeomTraits
#ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
  = typename Polyhedron::Traits
#endif
>
int 
segment_from_sdf_values( const Polyhedron& polyhedron, 
                         SDFPropertyMap sdf_values, 
                         SegmentPropertyMap segment_ids,
                         int number_of_clusters = 5, 
                         double smoothing_lambda = 0.26,
                         bool extract_segments = true,
                         GeomTraits traits = GeomTraits())
{ 
    internal::Surface_mesh_segmentation<Polyhedron, GeomTraits> algorithm(polyhedron, traits);
    return algorithm.partition(number_of_clusters, smoothing_lambda, sdf_values, segment_ids, extract_segments);
}

///\cond SKIP_IN_MANUAL
template < bool Fast_sdf_calculation_mode, class Polyhedron, class SegmentPropertyMap, class GeomTraits
#ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
  = typename Polyhedron::Traits
#endif
>
int 
compute_sdf_values_and_segment(const Polyhedron& polyhedron,
                           SegmentPropertyMap segment_ids,
                           double cone_angle = 2.0 / 3.0 * CGAL_PI,
                           int number_of_rays = 25,
                           int number_of_clusters = 5, 
                           double smoothing_lambda = 0.26,
                           bool extract_segments = true,
                           GeomTraits traits = GeomTraits())
{
    typedef std::map< typename Polyhedron::Facet_const_handle, double> Facet_double_map;
    Facet_double_map internal_sdf_map;
    boost::associative_property_map<Facet_double_map> sdf_property_map(internal_sdf_map);
    
    compute_sdf_values<Fast_sdf_calculation_mode, Polyhedron, boost::associative_property_map<Facet_double_map>, GeomTraits>
        (polyhedron, sdf_property_map, cone_angle, number_of_rays, true, traits);
    return segment_from_sdf_values<Polyhedron, boost::associative_property_map<Facet_double_map>, SegmentPropertyMap, GeomTraits>
        (polyhedron, sdf_property_map, segment_ids, number_of_clusters, smoothing_lambda, extract_segments, traits);
}
/// \endcond


/*!
 * \ingroup PkgSurfaceSegmentation
 * @brief Function computing the segmentation of a surface mesh.
 *
 * This function is equivalent to calling the functions `CGAL::compute_sdf_values()` and
 * `CGAL::segment_from_sdf_values()` with the same parameters.
 *
 * \note There is no direct relation between the parameter `number_of_clusters`
 * and the final number of segments after segmentation. However, setting a large number of clusters will result in a detailed segmentation of the mesh with a large number of segments.
 * \note For computing segmentations of the mesh with different parameters (i.e. number of levels, and smoothing lambda),
 * it is more efficient to first compute the SDF values using `CGAL::compute_sdf_values()` and use them in different calls to
 * `CGAL::segment_from_sdf_values()`.
 *
 * @pre @a polyhedron.is_pure_triangle()
 * @pre @a number_of_clusters > 0
 *
 * @tparam Polyhedron a %CGAL polyhedron
 * @tparam SegmentPropertyMap a `ReadWritePropertyMap` with `Polyhedron::Facet_const_handle` as key and `int` as value type
 * @tparam GeomTraits a model of SegmentationGeomTraits
 *
 * @param polyhedron surface mesh on which SDF values are computed
 * @param[out] segment_ids the segment or cluster id of each facet
 * @param cone_angle opening angle in radians for the cone of each facet
 * @param number_of_rays number of rays picked in the cone of each facet. In our experiments, we observe that increasing the number of rays beyond the default has a little effect on the quality of the segmentation result
 * @param number_of_clusters number of clusters for the soft clustering
 * @param smoothing_lambda factor which indicates the importance of the surface features for the energy minimization. It is recommended to choose a value in the interval [0,1]. See the section \ref Surface_mesh_segmentationGraphCut for more details.
 * @param extract_segments if `true` fill `segment_ids` with segment-ids and cluster-ids otherwise (see \cgalFigureRef{Cluster_vs_segment})
 * @param traits traits class
 *
 * @return number of segments if `extract_segments` is set to `true` and `number_of_clusters` otherwise
 */ 
template < class Polyhedron, class SegmentPropertyMap, class GeomTraits
#ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
  = typename Polyhedron::Traits
#endif
>
int 
compute_sdf_values_and_segment(const Polyhedron& polyhedron,                                
                           SegmentPropertyMap segment_ids,
                           double cone_angle = 2.0 / 3.0 * CGAL_PI,
                           int number_of_rays = 25,
                           int number_of_clusters = 5, 
                           double smoothing_lambda = 0.26,
                           bool extract_segments = true,
                           GeomTraits traits = GeomTraits())
{
    return compute_sdf_values_and_segment<true, Polyhedron, SegmentPropertyMap, GeomTraits>
      (polyhedron, segment_ids, cone_angle, number_of_rays, number_of_clusters, smoothing_lambda, extract_segments, traits);
}


#ifdef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
template <bool Fast_sdf_calculation_mode, class Polyhedron, class SDFPropertyMap>
std::pair<double, double> 
compute_sdf_values(const Polyhedron& polyhedron, 
                        SDFPropertyMap sdf_values, 
                        double cone_angle = 2.0 / 3.0 * CGAL_PI,
                        int number_of_rays = 25,
                        bool postprocess = true,
                        typename Polyhedron::Traits traits = typename Polyhedron::Traits())
{
    return compute_sdf_values<Fast_sdf_calculation_mode, Polyhedron, SDFPropertyMap, typename Polyhedron::Traits>
        (polyhedron, sdf_values, cone_angle, number_of_rays, postprocess, traits);
}

template < class Polyhedron, class SDFPropertyMap>
std::pair<double, double> 
compute_sdf_values(const Polyhedron& polyhedron, 
                        SDFPropertyMap sdf_values, 
                        double cone_angle = 2.0 / 3.0 * CGAL_PI,
                        int number_of_rays = 25,
                        bool postprocess = true,
                        typename Polyhedron::Traits traits = typename Polyhedron::Traits())
{
    return compute_sdf_values<true, Polyhedron, SDFPropertyMap, typename Polyhedron::Traits>
        (polyhedron, sdf_values, cone_angle, number_of_rays, postprocess, traits);
}

template <class Polyhedron, class SDFPropertyMap, class SegmentPropertyMap>
int 
segment_from_sdf_values(const Polyhedron& polyhedron, 
                                           SDFPropertyMap sdf_values, 
                                           SegmentPropertyMap segment_ids,
                                           int number_of_clusters = 5, 
                                           double smoothing_lambda = 0.26,
                                           bool extract_segments = true,
                                           typename Polyhedron::Traits traits = typename Polyhedron::Traits())
{
    return segment_from_sdf_values<Polyhedron, SDFPropertyMap, SegmentPropertyMap, typename Polyhedron::Traits>
        (polyhedron, sdf_values, segment_ids, number_of_clusters, smoothing_lambda, extract_segments, traits);
}

template <bool Fast_sdf_calculation_mode, class Polyhedron, class SegmentPropertyMap>
int 
compute_sdf_values_and_segment(const Polyhedron& polyhedron,                                
                           SegmentPropertyMap segment_ids,
                           double cone_angle = 2.0 / 3.0 * CGAL_PI,
                           int number_of_rays = 25,
                           int number_of_clusters = 5,
                           double smoothing_lambda = 0.26,
                           bool extract_segments = true,
                           typename Polyhedron::Traits traits = typename Polyhedron::Traits())
{
    return compute_sdf_values_and_segment< Fast_sdf_calculation_mode, Polyhedron, SegmentPropertyMap, typename Polyhedron::Traits>
        (polyhedron, segment_ids, cone_angle, number_of_rays, number_of_clusters, smoothing_lambda, extract_segments, traits);
}

template <class Polyhedron, class SegmentPropertyMap>
int 
compute_sdf_values_and_segment(const Polyhedron& polyhedron,                                
                           SegmentPropertyMap segment_ids,
                           double cone_angle = 2.0 / 3.0 * CGAL_PI,
                           int number_of_rays = 25,
                           int number_of_clusters = 5, 
                           double smoothing_lambda = 0.26,
                           bool extract_segments = true,
                           typename Polyhedron::Traits traits = typename Polyhedron::Traits())
{
    return compute_sdf_values_and_segment<true, Polyhedron, SegmentPropertyMap, typename Polyhedron::Traits>
        (polyhedron, segment_ids, cone_angle, number_of_rays, number_of_clusters, smoothing_lambda, extract_segments, traits);
}


#endif

}//namespace CGAL

#endif // CGAL_SURFACE_MESH_SEGMENTATION_MESH_SEGMENTATION_H //