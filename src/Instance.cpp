#include "../include/Instance.h"

#include <iostream>

void Instance::load(const string &filename)
{
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS)
    {
        cerr << "Erro ao carregar o arquivo XML: " << filename << endl;
        return;
    }

    XMLElement *root = doc.FirstChildElement("HighSchoolTimetableArchive");
    if (!root)
    {
        cerr << "Elemento raiz não encontrado!" << endl;
        return;
    }

    XMLElement *instance = root->FirstChildElement("Instances")->FirstChildElement("Instance");
    if (!instance)
    {
        cerr << "Instância não encontrada!" << endl;
        return;
    }

    // Carregar tempos
    XMLElement *times_elem = instance->FirstChildElement("Times");
    int time_count = 0;
    for (XMLElement *time_elem = times_elem->FirstChildElement("Time"); time_elem; time_elem = time_elem->NextSiblingElement("Time"))
    {
        TimeInfo t;
        t.id = time_elem->Attribute("Id");

        // Obter dia
        XMLElement *day_elem = time_elem->FirstChildElement("Day");
        if (day_elem)
        {
            t.day = day_elem->Attribute("Reference");
            if (t.day.find("gr_") == 0)
            {
                t.day = t.day.substr(3); // Remover "gr_"
            }
        }

        // Obter slot
        XMLElement *name_elem = time_elem->FirstChildElement("Name");
        if (name_elem && name_elem->GetText())
        {
            string name = name_elem->GetText();
            size_t pos = name.find('_');
            if (pos != string::npos)
            {
                t.slot = stoi(name.substr(pos + 1));
            }
        }

        // Verificar duração máxima
        t.max_duration = 1;
        XMLElement *timeGroups = time_elem->FirstChildElement("TimeGroups");
        if (timeGroups)
        {
            XMLElement *tg = timeGroups->FirstChildElement("TimeGroup");
            while (tg)
            {
                if (tg->Attribute("Reference") && string(tg->Attribute("Reference")) == "gr_TimesDurationTwo")
                {
                    t.max_duration = 2;
                    break;
                }
                tg = tg->NextSiblingElement("TimeGroup");
            }
        }

        times.push_back(t);
        time_index[t.id] = time_count++;
    }

    // Carregar recursos
    XMLElement *resources_elem = instance->FirstChildElement("Resources");
    for (XMLElement *res_elem = resources_elem->FirstChildElement("Resource"); res_elem; res_elem = res_elem->NextSiblingElement("Resource"))
    {
        ResourceInfo r;
        r.id = res_elem->Attribute("Id");

        XMLElement *name_elem = res_elem->FirstChildElement("Name");
        if (name_elem && name_elem->GetText())
        {
            r.name = name_elem->GetText();
        }

        XMLElement *type_elem = res_elem->FirstChildElement("ResourceType");
        if (type_elem && type_elem->Attribute("Reference"))
        {
            r.type = type_elem->Attribute("Reference");
        }

        resources.push_back(r);

        if (r.type == "Teacher")
        {
            teacher_name[r.id] = r.name;
        }
    }

    // Carregar eventos
    XMLElement *events_elem = instance->FirstChildElement("Events");
    int event_count = 0;
    for (XMLElement *event_elem = events_elem->FirstChildElement("Event"); event_elem; event_elem = event_elem->NextSiblingElement("Event"))
    {
        EventInfo e;
        e.id = event_elem->Attribute("Id");

        XMLElement *duration_elem = event_elem->FirstChildElement("Duration");
        if (duration_elem && duration_elem->GetText())
        {
            e.total_duration = atoi(duration_elem->GetText());
        }

        XMLElement *course_elem = event_elem->FirstChildElement("Course");
        if (course_elem && course_elem->Attribute("Reference"))
        {
            e.course_id = course_elem->Attribute("Reference");
        }

        // Obter recursos (professor e turma)
        XMLElement *resources = event_elem->FirstChildElement("Resources");
        if (resources)
        {
            for (XMLElement *res_elem = resources->FirstChildElement("Resource"); res_elem; res_elem = res_elem->NextSiblingElement("Resource"))
            {
                XMLElement *role_elem = res_elem->FirstChildElement("Role");
                if (role_elem && role_elem->GetText())
                {
                    string role = role_elem->GetText();
                    string res_id = res_elem->Attribute("Reference");

                    if (role == "Teacher")
                        e.teacher_id = res_id;
                    else if (role == "Class")
                        e.class_id = res_id;
                }
            }
        }

        events.push_back(e);
        event_index[e.id] = event_count++;
    }

    // Carregar restrições
    XMLElement *constraints_elem = instance->FirstChildElement("Constraints");
    for (XMLElement *constr_elem = constraints_elem->FirstChildElement(); constr_elem; constr_elem = constr_elem->NextSiblingElement())
    {
        ConstraintInfo c;
        c.type = constr_elem->Value();

        XMLElement *required_elem = constr_elem->FirstChildElement("Required");
        if (required_elem && required_elem->GetText())
        {
            c.required = (string(required_elem->GetText()) == "true");
        }

        XMLElement *weight_elem = constr_elem->FirstChildElement("Weight");
        if (weight_elem && weight_elem->GetText())
        {
            c.weight = atoi(weight_elem->GetText());
        }

        // Aplicação da restrição
        XMLElement *applies_to = constr_elem->FirstChildElement("AppliesTo");
        if (applies_to)
        {
            // Eventos
            XMLElement *event_groups = applies_to->FirstChildElement("EventGroups");
            if (event_groups)
            {
                for (XMLElement *eg = event_groups->FirstChildElement("EventGroup"); eg; eg = eg->NextSiblingElement("EventGroup"))
                {
                    if (eg->Attribute("Reference"))
                    {
                        c.applies_to_events.insert(eg->Attribute("Reference"));
                    }
                }
            }

            // Professores
            XMLElement *teachers = applies_to->FirstChildElement("Resources");
            if (teachers)
            {
                for (XMLElement *t = teachers->FirstChildElement("Resource"); t; t = t->NextSiblingElement("Resource"))
                {
                    if (t->Attribute("Reference"))
                    {
                        c.applies_to_teachers.insert(t->Attribute("Reference"));
                    }
                }
            }

            // Turmas
            XMLElement *classes = applies_to->FirstChildElement("ResourceGroups");
            if (classes)
            {
                for (XMLElement *cl = classes->FirstChildElement("ResourceGroup"); cl; cl = cl->NextSiblingElement("ResourceGroup"))
                {
                    if (cl->Attribute("Reference"))
                    {
                        c.applies_to_classes.insert(cl->Attribute("Reference"));
                    }
                }
            }

            // Horários
            XMLElement *times = applies_to->FirstChildElement("Times");
            if (times)
            {
                for (XMLElement *tm = times->FirstChildElement("Time"); tm; tm = tm->NextSiblingElement("Time"))
                {
                    if (tm->Attribute("Reference"))
                    {
                        c.applies_to_times.insert(tm->Attribute("Reference"));
                    }
                }
            }
        }

        // Parâmetros específicos
        if (c.type == string("DistributeSplitEventsConstraint"))
        {
            XMLElement *duration_elem = constr_elem->FirstChildElement("Duration");
            if (duration_elem && duration_elem->GetText())
            {
                c.duration_constraint = atoi(duration_elem->GetText());
            }

            XMLElement *min_elem = constr_elem->FirstChildElement("Minimum");
            if (min_elem && min_elem->GetText())
            {
                c.min_value = atoi(min_elem->GetText());
            }

            XMLElement *max_elem = constr_elem->FirstChildElement("Maximum");
            if (max_elem && max_elem->GetText())
            {
                c.max_value = atoi(max_elem->GetText());
            }

            for (const string &course_id : c.applies_to_events)
            {
                course_split_constraints[course_id] = make_pair(c.min_value, c.max_value);
            }
        }
        else if (c.type == string("ClusterBusyTimesConstraint"))
        {
            XMLElement *min_elem = constr_elem->FirstChildElement("Minimum");
            if (min_elem && min_elem->GetText())
            {
                c.min_value = atoi(min_elem->GetText());
            }

            XMLElement *max_elem = constr_elem->FirstChildElement("Maximum");
            if (max_elem && max_elem->GetText())
            {
                c.max_value = atoi(max_elem->GetText());
            }

            for (const string &teacher_id : c.applies_to_teachers)
            {
                teacher_max_days[teacher_id] = c.max_value;
            }
        }
        else if (c.type == string("AvoidUnavailableTimesConstraint"))
        {
            for (const string &teacher_id : c.applies_to_teachers)
            {
                for (const string &time_id : c.applies_to_times)
                {
                    teacher_unavailable_times[teacher_id].insert(time_id);
                }
            }
        }

        constraints.push_back(c);
    }
}